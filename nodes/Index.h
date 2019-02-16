# pragma once

#include <climits>
#include <mutex>

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
        m_head(std::make_shared<HeadIndex>(head_node, std::shared_ptr<HeadIndex>(), std::shared_ptr<IndexNode>(), 1)),
        m_rand(2, (1 << 30) - 1)
    { }

    friend std::ostream& operator<< (std::ostream& stream, const Index<key_t, val_t>& index) {
        auto cur = index.m_head;
        while(cur) {
            auto level = cur->m_level;
            stream << "level: " << level;
            auto r = cur->m_right;
            while (r) {
                stream << "\t" << cur->m_node << ",";
//                if (r->m_down)
//                    stream << "v";
                r = r->m_right;
            }
            stream << "\tNone\n";
            cur = cur->m_down;
        }
        return stream;
    }

    void add(node_t node_to_add) {
        if (node_to_add.is_null())
            throw std::invalid_argument("NULL pointer node was given to Index::add");
        int rnd = m_rand.get();
        int level = 1, max;
        while (((rnd >>= 1) & 1) != 0)
            ++level;
        auto head = m_head;
        int old_level = head->m_level;
        level = std::min(level, old_level + 1); // always try to grow by at most one level
        std::shared_ptr<IndexNode> idx = NULL;
        std::vector<std::shared_ptr<IndexNode>> idxs(level+1);

        for (int i = 1; i <= level; ++i)
            idxs[i] = idx = std::make_shared<IndexNode>(node_to_add, idx, std::shared_ptr<IndexNode>());
        if (old_level < level) { // try to grow
            for (; ; ) {
                head = m_head;
                int old_level = head->m_level;
                if (level <= old_level) // lost race to add level
                    break;
                auto newh = head;
                node_t old_base = head->m_node;
                for (int j = old_level + 1; j <= level; ++j) // maybe reduce level happened in between?
                    newh = std::make_shared<HeadIndex>(old_base, newh, idxs[j], j);
                if (casHead(head, newh)) {
                    head = newh;
                    idx = idxs[level = old_level]; // level is changed because idxs in all levels b/w old_level to level was already insert
                    break;
                }
            }
        }

        // find insertion points and insert
        bool found;
        std::shared_ptr<IndexNode> curr = head;
        auto r = curr->m_right;
        auto t = idx;
        auto j = head->m_level;
        for (int insertion_level = level; ; ) {
            if (!curr || !t)
                return;
            for (; ; ) {
                std::tie(found, curr, r) = walkLevel(curr, node_to_add);
                if (found)
                    break;
            }

            if (j == insertion_level) {
                if (!curr->link(r, t))
                    continue; // link failed, restart
                if (!t->m_node->m_val || --insertion_level == 0)
                    return;
            }

            if (--j >= insertion_level && j < level)
                t = t->m_down;
            curr = curr->m_down;
            if (curr)
                r = curr->m_right;
        }
    }

    void remove(node_t node) {
        if (node == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::remove");
        findPredecessor(node); // clean index
        if (m_head.m_right == NULL)
            tryReduceLevel();
    }

private:
    Rand m_rand;
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
            auto node = m_node;
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
        HeadIndex(node_t node, std::shared_ptr<HeadIndex> down, std::shared_ptr<IndexNode> right, uint64_t level):
                IndexNode(node, down, right),
                m_down(down),
                m_level(level) { }
    };

    /**
     * compareAndSet head node
     */
    bool casHead(std::shared_ptr<HeadIndex> cmp, std::shared_ptr<HeadIndex> val) {
        std::lock_guard<std::mutex> l(m_lock);
        if (m_head == cmp) {
            m_head = val;
            return true;
        } return false;
    }

    /**
     * Returns a node with key strictly less than given key,
     * or the header if there is no such node.  Also
     * unlinks indexes to deleted nodes found along the way.  Callers
     * rely on this side-effect of clearing indices to deleted nodes.
     *
     * @return a predecessor of key
     */
    node_t findPredecessor(node_t node) {
        while (true) {
            for (IndexNode q = m_head, r = q.m_right, d; ; ) {
                if (r != NULL) {
                    node_t n = r.m_node;
                    if (n.m_val == NULL) { // this node is deleted - needs to be unlinked
                        if (!q.unlink(r))
                            break;           // restart
                        r = q.m_right;         // reread r
                        continue;
                    }
                    if (node.m_key > n.m_key) { // continue in the same level
                        q = r;
                        r = r.m_right;
                        continue;
                    }
                }
                if ((d = q.m_down) == NULL) // no more levels left - we found the closest one
                    return q.m_node;
                q = d;
                r = d.m_right;
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
        HeadIndex h = m_head; // TODO shared ptr way?
        HeadIndex d;
        HeadIndex e;
        if (h.m_level > 3 &&
            (d = (HeadIndex) h.m_down) != NULL &&
            (e = (HeadIndex) d.m_down) != NULL &&
            e.m_right == NULL &&
            d.m_right == NULL &&
            h.m_right == NULL &&
            casHead(h, d) && // try to set
            h.m_right != NULL) // recheck
            casHead(d, h);   // try to backout
    }

    std::tuple<bool, std::shared_ptr<IndexNode>, std::shared_ptr<IndexNode>> walkLevel
            (std::shared_ptr<IndexNode> head, node_t node_to_add) {
        if (!head)
            throw std::invalid_argument("NULL pointer head was given to Index::walkOnLevel");
        auto q = head;
        auto r = q->m_right;
        while (r) {
            node_t n = r->m_node;
            // compare before deletion check avoids needing recheck
            bool c = (node_to_add->m_key > n->m_key);
            if (!n->m_val) { // need to unlink deleted node
                if (!q->unlink(r))
                    return std::make_tuple(false, std::shared_ptr<IndexNode>(), std::shared_ptr<IndexNode>());
            } else if (c)
                q = r;
            else
                break;
            r = q->m_right;
        }
        return std::make_tuple(true, q, r);
    }

    node_t getPred(node_t node) {
        if (node == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::getPred");
        for (; ; ) {
            node_t b = findPredecessor(node);
            if (b.m_val != NULL) // not deleted
                return b;
        }
    }

    std::shared_ptr<HeadIndex> m_head;
};
