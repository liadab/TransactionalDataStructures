# pragma once

#include <climits>
#include "utils.h"
#include "LNode.h"
#include "LNodeWrapper.h"


template <typename key_t, typename val_t>
class Index {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    Index(node_t head_node) {
        head_node->m_val = BASE_HEADER;
        m_head = new HeadIndex(head_node, NULL, NULL, 1);
        m_rand = new Rand(2, 2^31 - 1); // distribution 
    }

    void add(LNode<key_t, val_t> node_to_add) {
        LNode<key_t, val_t> node = node_to_add;
        if (node == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::add");
        int rnd = m_rand.get();
//        if ((rnd & 0x80000001) != 0) return; // test highest and lowest bits - TODO: why?? why not change the distribution?
        int level = 1, max;
        while (((rnd >>= 1) & 1) != 0)
            ++level;
        IndexNode idx = NULL;
        HeadIndex head = m_head;
        if ((max = head.m_level) >= level) {
            for (int i = 1; i <= level; ++i)
                idx = new IndexNode(node, idx, NULL);
        } else { // try to grow by one level
            level = max + 1; // hold in array and later pick the one to use
            IndexNode idxs[] = new IndexNode[level + 1];
            for (int i = 1; i <= level; ++i)
                idxs[i] = idx = new IndexNode(node, idx, NULL);
            for (; ; ) {
                head = m_head;
                int old_level = head.m_level;
                if (level <= old_level) // lost race to add level
                    break;
                HeadIndex newh = head;
                LNode old_base = head.m_node;
                for (int j = old_level + 1; j <= level; ++j)
                    newh = new HeadIndex(old_base, newh, idxs[j], j);
                if (casHead(head, newh)) {
                    head = newh;
                    idx = idxs[level = old_level]; // level is changed because idxs in all levels b/w ol_level to level was already insert
                    break;
                }
            }
        }
        // find insertion points and splice in splice:
        for (int insertion_level = level; ; ) {
            int j = head.m_level;
            for (IndexNode q = head, r = q.m_right, t = idx; ; ) {
                if (q == NULL || t == NULL)
                    break splice;
                if (r != NULL) {
                    LNode<key_t, val_t> n = r.m_node;
                    // compare before deletion check avoids needing recheck
                    bool c = (node.m_key > n.m_key);
                    if (n..m_val() == NULL) {
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
                    if (t.node..m_val() == NULL) {
                        break splice;
                    }
                    if (--insertion_level == 0)
                        break splice;
                }

                if (--j >= insertion_level && j < level)
                    t = t.m_down;
                q = q.m_down;
                r = q.m_right;
            }
        }
    }

    void remove(LNode<key_t, val_t> node) {
        if (node == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::remove");
        findPredecessor(node); // clean index
        if (m_head.m_right == NULL)
            tryReduceLevel();
    }

private:
    static val_t BASE_HEADER = new val_t(); // TODO
    Rand m_rand;

    class IndexNode {
    public:
        // TODO: should I need to implement the UNSAFE mechanism?
        std::unique_ptr<IndexNode> m_right;
        std::unique_ptr<IndexNode> m_down;
        std::unique_ptr<LNode<key_t, val_t>> m_node;

        IndexNode(LNode<key_t, val_t> node, IndexNode down, IndexNode right) {
            m_node = node;
            m_down = down;
            m_right = right;
        }

        bool casRight(IndexNode cmp, IndexNode val) {
            // TODO: cas?
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
    };

    class HeadIndex : public IndexNode {
    public:
        uint64_t m_level; //TODO: final?

        HeadIndex(LNode<key_t, val_t> node, IndexNode down, IndexNode right, uint64_t level):
                IndexNode(node, down, right) {
            m_level = level;
        }
    };

    /**
     * compareAndSet head node
     */
    bool casHead(HeadIndex cmp, HeadIndex val) {
        // TODO: ?
        return UNSAFE.compareAndSwapObject(this, headOffset, cmp, val);
    }

    /**
     * Returns a node with key strictly less than given key,
     * or the header if there is no such node.  Also
     * unlinks indexes to deleted nodes found along the way.  Callers
     * rely on this side-effect of clearing indices to deleted nodes.
     *
     * @return a predecessor of key
     */
    LNode<key_t, val_t> findPredecessor(LNode<key_t, val_t> node) {
        while (true) {
            for (IndexNode q = m_head, r = q.m_right, d; ; ) {
                if (r != NULL) {
                    LNode<key_t, val_t> n = r.m_node;
                    if (n..m_val() == NULL) { // this node is deleted - needs to be unlinked
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
        HeadIndex h = head;
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

    LNode<key_t, val_t> getPred(LNode<key_t, val_t> node) {
        if (node == NULL)
            throw std::invalid_argument("NULL pointer node was given to Index::getPred");
        for (; ; ) {
            LNode<key_t, val_t> b = findPredecessor(node);
            if (b..m_val() != NULL) // not deleted
                return b;
        }
    }

    HeadIndex m_head; // TODO: volatile?
};
#include "Index.inl"