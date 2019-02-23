/* TODO list
 * make constructor init head's val as min - and understand why 0 == NULL but (!0) returns false?
 * */

# pragma once

#include <climits>
#include <mutex>
#include <vector>

#include "utils.h"
#include "LNode.h"
#include "LNodeWrapper.h"


template <typename key_t, typename val_t>
class Index {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    /**
     * Index initializer
     * @param head_node    assumed to be dummy node
     */
    Index(node_t head_node) :
        m_head_bottom(std::make_shared<HeadIndex>(head_node, std::shared_ptr<HeadIndex>(), std::shared_ptr<IndexNode>(), 1))
    {
        m_head_top = m_head_bottom;
    }

    friend std::ostream& operator<< (std::ostream& stream, const Index<key_t, val_t>& index) {
        auto cur = index.m_head_top;
        while(cur) {
            auto level = cur->m_level;
            stream << "level: " << level;
            std::shared_ptr<IndexNode> r = cur;
            while (r) {
                stream << "\t" << r->m_node;
                if (r->m_down)
                    stream << "v";
                else
                    stream << "x";
                stream << ",";
                r = r->m_right;
            }
            stream << "\tNone\n";
            cur = cur->m_down;
        }
        cur = index.m_head_bottom;
        stream << "bottom level: " << cur->m_level << " node: " << cur->m_node;
        return stream;
    }

    /**
     * adds node to the index
     *
     * @param node_to_add    the node to be added
     */
    void add(node_t node_to_add) {
        // TODO: make findPred find all the insertion points, and walk from start to end only when cas fails! so it will be more efficient
        if (node_to_add.is_null())
            throw std::invalid_argument("NULL pointer node was given to Index::add");
        int rnd = get_random_in_range(2, (1 << 30) - 1);
        int level = 1, max;
        while (((rnd >>= 1) & 1) != 0)
            ++level;
        int old_level = m_head_top->m_level;
        level = std::min(level, old_level + 1); // always try to grow by at most one level
        std::shared_ptr<IndexNode> idx = NULL;
        std::vector<std::shared_ptr<IndexNode>> idxs(level+1);

        // create the new nodes
        for (int i = 1; i <= level; ++i) {
            idxs[i] = idx = std::make_shared<IndexNode>(node_to_add, idx, std::shared_ptr<IndexNode>());
        }

        // find insertion points in the existing levels - from bottom up
        bool finish;
        auto head = m_head_bottom;
        std::shared_ptr<IndexNode> curr = head;
        auto r = curr->m_right;
        auto t = idxs[0];
        int insertion_level = 0;
        while (head && head->m_level <= level) {
            for (; ; ) { // continously try to insert in level
                std::tie(finish, curr, r) = walkLevel(head, node_to_add);
                if (finish)
                    break;
            }
            t = idxs[head->m_level];
            if (!curr->link(r, t))
                continue; // link failed, restart
            if (!t->m_node->m_val)
                return; // node is exactly being deleted, abort
            head = head->m_up;
            insertion_level++;
        }

        if (insertion_level == level) // no need to continue
            return;

        old_level = m_head_top->m_level; // maybe in the meanwhile things have changed..
        if (old_level < level) {
            // try to grow - as before only by one level at a time!
            head = m_head_top;
            old_level = head->m_level;
            if (level <= old_level) // lost race to add level
                return;
            node_t old_base = head->m_node;
            auto newh = std::make_shared<HeadIndex>(old_base, head, idxs[old_level + 1], old_level + 1);
            if (casHead(head, newh, true)) {
                head = newh;
                idx = idxs[level = old_level]; // level is changed because idxs in all levels b/w old_level to level was already insert
                return;
            }
        } // else - maybe we lost some insertion level, not so bad..
    }

    /**
     * removes node from the index.
     * node's val assumed to be null
     *
     * @param node    the node to be removed
     */
    void remove(node_t node) {
        if (node.is_null()) {
            throw std::invalid_argument("NULL pointer node was given to Index::remove");
        }
        if (node->m_val) {
            // TODO delete this
            throw std::invalid_argument("Index::remove got a non-NULL node to remove");
        }
        std::cout << "removing key:" << node->m_key << std::endl;
        findPredecessor(node); // clean index
        if (!m_head_top->m_right) {
            tryReduceLevel();
        }
    }

    node_t getPred(node_t node) {
        if (node.is_null()) {
            throw std::invalid_argument("NULL pointer node was given to Index::getPred");
        }
        for (; ; ) {
            node_t b = findPredecessor(node);
            if (b->m_val) // not deleted
            {
                return b;
            }
        }
    }

private:
    std::mutex m_lock;

    class IndexNode {
    public:
        std::shared_ptr<IndexNode> m_right;
        const std::shared_ptr<IndexNode> m_down;
        const node_t m_node;

        IndexNode(node_t node, std::shared_ptr<IndexNode> down, std::shared_ptr<IndexNode> right)
        : m_node(node), m_down(down), m_right(right) { }

        bool casRight(std::shared_ptr<IndexNode> cmp, std::shared_ptr<IndexNode> val) {
            std::lock_guard<std::mutex> l(m_lock);
            if (m_right == cmp) {
                m_right = val;
                return true;
            } return false;
        }

        /**
         * Tries to CAS newSucc as successor.  To minimize races with
         * unlink that may lose this index node, if the node being
         * indexed is known to be deleted, it doesn't try to link in.
         *
         * @param succ    the expected current successor
         * @param new_succ the new successor
         * @return true if successful
         */
        bool link(std::shared_ptr<IndexNode> succ, std::shared_ptr<IndexNode> new_succ) { // TODO: final??
            new_succ->m_right = succ;
            return m_node->m_val && casRight(succ, new_succ);
        }

        /**
         * Tries to CAS right field to skip over apparent successor
         * succ.  Fails (forcing a retraversal by caller) if this node
         * is known to be deleted.
         *
         * @param succ the expected current successor
         * @return true if successful
         */
        bool unlink(std::shared_ptr<IndexNode> succ) { // TODO final
            if (!succ)
                return true;
            return m_node->m_val && casRight(succ, succ->m_right);
        }

    private:
        std::mutex m_lock;
    };

    class HeadIndex : public IndexNode {
    public:
        const uint64_t m_level;
        const std::shared_ptr<HeadIndex> m_down;
        std::shared_ptr<HeadIndex> m_up;
        HeadIndex(node_t node, std::shared_ptr<HeadIndex> down, std::shared_ptr<IndexNode> right, uint64_t level):
                IndexNode(node, down, right),
                m_down(down),
                m_up(std::shared_ptr<HeadIndex>()),
                m_level(level) { }
    };

    /**
     * compareAndSet head node
     */
    bool casHead(std::shared_ptr<HeadIndex> cmp, std::shared_ptr<HeadIndex> val, bool up) {
        std::lock_guard<std::mutex> l(m_lock);
        if (m_head_top == cmp) {
            if (up) _levelUp(cmp, val);
            else _levelDown(cmp, val);
            return true;
        } return false;
    }

    /**
     * level up m_top_head to val, make old head points to the new one as well
     * m_lock is assumed to be taken,
     */
    bool _levelUp(std::shared_ptr<HeadIndex> cmp, std::shared_ptr<HeadIndex> val) {
        assert(val->m_down == cmp && !val->m_up);
        cmp->m_up = val;
        m_head_top = val;
    }

    /**
     * level down m_top_head to val
     * m_lock is assumed to be taken,
     */
    bool _levelDown(std::shared_ptr<HeadIndex> cmp, std::shared_ptr<HeadIndex> val) {
        val->m_up = std::shared_ptr<HeadIndex>();
        m_head_top = val;
    }

    /**
     * Returns a node with key strictly less than given key,
     * or the header if there is no such node.  Also
     * unlinks indexes to deleted nodes found along the way.  Callers
     * rely on this side-effect of clearing indices to deleted nodes.
     *
     * @return a predecessor of key
     */
    node_t findPredecessor(node_t node_to_find) {
        while (true) {
            bool finish;
            std::shared_ptr<IndexNode> prev;
            std::shared_ptr<IndexNode> dumm;
            std::shared_ptr<IndexNode> curr = m_head_top;
            auto r = curr->m_right;
            auto d = curr->m_down;
            auto level = m_head_top->m_level;
            while (level) {

                if (!curr->m_node->m_val) { // maybe curr was unlinked. start the whole operation over!
                    break;
                }
                for (;;) {
                    std::tie(finish, prev, dumm) = walkLevel(curr, node_to_find);
                    if (finish) break;
                }
                auto node = prev->m_node;
                if (!node->m_val) // node prev is about to be removed, restart level
                    continue;
                if (!(d = prev->m_down)) // no more levels left - we found the closest one
                    return node;
                curr = d;
                level--;
            }
        }
    }

    /**
     * Possibly reduce head level if it has no nodes.  This method can
     * (rarely) make mistakes, in which case levels can disappear even
     * though they are about to contain index nodes. This impacts
     * performance, not correctness.  To minimize mistakes as well as
     * to reduce hysteresis, the level is reduced by one only if the
     * topmost three levels look empty. Also, if the removed level
     * looks non-empty after CAS, we try to change it back quick
     * before anyone notices our mistake! (This trick works pretty
     * well because this method will practically never make mistakes
     * unless current thread stalls immediately before first CAS, in
     * which case it is very unlikely to stall again immediately
     * afterwards, so will recover.)
     * <p>
     * We put up with all this rather than just let levels grow
     * because otherwise, even a small map that has undergone a large
     * number of insertions and removals will have a lot of levels,
     * slowing down access more than would an occasional unwanted
     * reduction.
     */
    void tryReduceLevel() {
        // TODO: validate it, add debug prints
        auto h = m_head_top;
        if (h->m_level < 3) {
            return;
        }
        auto d = h->m_down;
        auto e = d->m_down;
        if (
                d && e &&
                !e->m_right &&
                !d->m_right &&
                !h->m_right &&
                casHead(h, d, false) && // try to set
                h->m_right
            ) { // recheck
            casHead(d, h, true);   // try to backout
        }
    }

    /**
     * walk on one level, starts with start, until the predecessor (smaller or equal) of node
     *
     * @param start the node to start the search from
     * @param node_to_add node to search
     * @return a tuple: is the search was finished (or needs to restart), predecessor, predecessor's right
     * */
    std::tuple<bool, std::shared_ptr<IndexNode>, std::shared_ptr<IndexNode>> walkLevel
            (std::shared_ptr<IndexNode> start, node_t node_to_add) {
        if (!start)
            throw std::invalid_argument("NULL pointer head was given to Index::walkOnLevel");
        auto q = start;
        auto r = q->m_right;
        while (r) {
            node_t n = r->m_node;
            // compare before deletion check avoids needing recheck
            bool c = (node_to_add->m_key > n->m_key);
            if (!n->m_val) { // need to unlink deleted node
                if (!q->unlink(r)) { // need to restart walk..
                    std::cout << "unlink failed" << r << std::endl;
                    return std::make_tuple(false, std::shared_ptr<IndexNode>(), std::shared_ptr<IndexNode>());
                }
            } else if (c) {
                q = r;
            } else break;
            r = q->m_right;
        }
        return std::make_tuple(true, q, r);
    }


    std::shared_ptr<HeadIndex> m_head_top;
    const std::shared_ptr<HeadIndex> m_head_bottom;
};
