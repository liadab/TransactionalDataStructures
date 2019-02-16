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
    Index(node_t head_node) {
        m_head = new HeadIndex(head_node, NULL, NULL, 1);
        m_rand = new Rand(2, 1 << 31 - 1); // distribution
    }

    void add(node_t node_to_add) {
        if (node_to_add == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::add");
        int rnd = m_rand.get();
        int level = 1, max;
        while (((rnd >>= 1) & 1) != 0)
            ++level;
        auto head = m_head;
        int old_level = head.m_level;
        level = std::min(level, old_level + 1); // always try to grow by at most one level
        std::shared_ptr<IndexNode> idx = NULL;
        std::vector<std::shared_ptr<IndexNode>> idxs(level+1);

        for (int i = 1; i <= level; ++i)
            idxs[i] = idx = std::make_shared<IndexNode>(node_to_add, idx, NULL);
        if (old_level < level) {
            for (; ; ) {
                head = m_head;
                int old_level = head.m_level;
                if (level <= old_level) // lost race to add level
                    break;
                auto newh = head;
                node_t old_base = head.m_node;
                for (int j = old_level + 1; j <= level; ++j) // maybe reduce level happened in between
                    newh = std::make_shared<HeadIndex>(old_base, newh, idxs[j], j);
                if (casHead(head, newh)) {
                    head = newh;
                    idx = idxs[level = old_level]; // level is changed because idxs in all levels b/w old_level to level was already insert
                    break;
                }
            }
        }

        // find insertion points and splice in splice:
        for (int insertion_level = level; ; ) {
            int j = head.m_level;
            for (IndexNode q = head, r = q.m_right, t = idx; ; ) {
                if (q == NULL || t == NULL)
                    break; // TODO break slice
                if (r != NULL) {
                    node_t n = r.m_node;
                    // compare before deletion check avoids needing recheck
                    bool c = (node_to_add.m_key > n.m_key);
                    if (n.m_val == NULL) {
                        if (!q.unlink(r))
                            break;
                        r = q.m_right;
                        continue;
                    }
                    if (c) {
                        q = r;
                        r = r.m_right;
                        continue;
                    }
                }

                if (j == insertion_level) {
                    if (!q.link(r, t))
                        break; // restart
                    if (t.node.m_val == NULL) {
                        break; // TODO splice;
                    }
                    if (--insertion_level == 0)
                        break; // TODO splice;
                }

                if (--j >= insertion_level && j < level)
                    t = t.m_down;
                q = q.m_down;
                r = q.m_right;
            }
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
        // TODO: should I need to implement the UNSAFE mechanism?
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
        bool link(IndexNode succ, IndexNode new_succ) { // TODO: final??
            LNode<key_t, val_t> node = m_node;
            new_succ.m_right = succ;
            return m_node.get_val() != NULL && casRight(succ, new_succ);
        }

    private:
        std::mutex m_lock;
    };

    class HeadIndex : public IndexNode {
    public:
        const uint64_t m_level;
        HeadIndex(node_t node, std::shared_ptr<IndexNode> down, std::shared_ptr<IndexNode> right, uint64_t level):
                IndexNode(node, down, right),  m_level(level) { }
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
