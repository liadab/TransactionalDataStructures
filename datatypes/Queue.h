#pragma once

#include <mutex>
#include <atomic>

#include "../nodes/QNode.h"
#include "../LocalStorage.h"
#include "LocalQueue.h"
#include "../TX.h"

template <typename val_t>
class Queue {
public:
    // TODO : replace all QNode with QNode wrapper
    std::shared_ptr<TX> m_tx;
    QNode<val_t> m_head;
    QNode<val_t> m_tail;
    int m_size;
    static constexpr uint64_t LOCK_MASK = 0x1000000000000000L;
    static constexpr uint64_t SINGLETON_MASK = 0x4000000000000000L;
    static constexpr uint64_t VERSIONNEG_MASK = LOCK_MASK | SINGLETON_MASK;

    std::atomic<uint64_t> m_version_mask;

    explicit Queue(std::shared_ptr<TX> tx) :
    m_tx(std::move(tx)),
    m_size(0),
    m_head(NULLOPT),
    m_tail(NULLOPT),
    m_version_mask(0)
    { }

    bool operator==(const Queue<val_t>& other) const {
        return std::tie(m_tx.get(), m_size, m_head, m_tail, m_version_mask) ==
                std::tie(other.m_tx.get(), other.m_size, other.m_head, other.m_tail, other.m_version_mask);
    }

    bool operator!=(const LNodeWrapper<key_t, val_t>& other) const{
        return std::tie(m_tx.get(), m_size, m_head, m_tail, m_version_mask) !=
               std::tie(other.m_tx.get(), other.m_size, other.m_head, other.m_tail, other.m_version_mask);
    }

    uint64_t getVersion() {
        return (m_version_mask & (~VERSIONNEG_MASK));
    }

    void setVersion(uint64_t version) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "setVersion on unlocked queue");
        l &= VERSIONNEG_MASK;
        l |= (version & (~VERSIONNEG_MASK));
        m_version_mask = l;
    }

    bool isSingleton() {
        return (m_version_mask & SINGLETON_MASK) != 0;
    }

    void setSingleton(bool value) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "setSingleton on unlocked queue");
        if (value) {
            l |= SINGLETON_MASK;
        } else {
            l &= (~SINGLETON_MASK);
        }
        m_version_mask = l;
    }

    bool tryLock() {
        uint64_t l = m_version_mask;
        if ((l & LOCK_MASK) != 0) {
            return false;
        }
        uint64_t locked = l | LOCK_MASK;
        return m_version_mask.compare_exchange_strong(l, locked);
    }

    void unlock() {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "unlocking queue that is not locked");
        uint64_t  unlocked = l & (~LOCK_MASK);
        bool ret = m_version_mask.compare_exchange_strong(l, unlocked);
        assert (ret && "compare_exchange_strong in unlock queue failed");
    }

    bool isLocked() {
        uint64_t l = m_version_mask;
        return (l & LOCK_MASK) != 0;
    }

    void validateTxSafe() {
        if (m_tx->get_local_transaction().readVersion < getVersion()) {
            // abort TX
            m_tx->get_local_transaction().TX = false;
            throw TxAbortException();
        }
        if (m_tx->get_local_transaction().readVersion == getVersion() && isSingleton()) {
            // TODO in the case of a thread running singleton and then TX
            // this TX will abort once but for no reason
            m_tx->incrementAndGetVersion();
            m_tx->get_local_transaction().TX = false;
            throw TxAbortException();
        }
    }

    void enqueue(val_t val) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();

        // SINGLETON
        if (!m_tx->get_local_transaction().TX) {
            return enqueueSingleton(val);
        }

        // TX
        if (m_tx->DEBUG_MODE_QUEUE) {
            std::cout << "Queue enqueue - in TX" << std::endl;
        }

        validateTxSafe();

        auto q_map = localStorage.queueMap;
        auto l_queue = q_map.find(this);
        if (!l_queue) {
            l_queue = LocalQueue<val_t>(); // TODO: validate this is the right way
        }
        l_queue.enqueue(val);
        q_map.put(this, l_queue);
    }



    void enqueueSingleton(val_t val) {
        std::shared_ptr<QNode<val_t>> node = std::make_shared<QNode<val_t>>(val);
        while (true) {
            if (tryLock()) {
                if (m_tx->DEBUG_MODE_QUEUE) {
                    std::cout << "enqueueSingleton: val " << val << std::endl;
                }
                if (!m_tail) {
                    m_tail = node;
                    m_head = node;
                } else {
                    m_tail.m_next = node;
                    node.m_prev = m_tail;
                    m_tail = node;
                }
                m_size++;
                setVersion(m_tx->getVersion());
                setSingleton(true);
                unlock();
                return;
            }
        }
    }

    val_t dequeue() {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();

        // SINGLETON
        if (!m_tx->get_local_transaction().TX) {
            return dequeueSingleton();
        }

        // TX
        if (m_tx->DEBUG_MODE_QUEUE) {
            std::cout << "Queue dequeue - in TX" << std::endl;
        }

        validateTxSafe();

        if (!tryLock()) {
            // queue is locked by another thread - abort
            m_tx->get_local_transaction().TX = false;
            throw TxAbortException();
        }

        // now queue is locked
        auto q_map = localStorage.queueMap;
        auto l_queue = q_map.find(this);
        if (!l_queue) {
            l_queue = LocalQueue<val_t>(); // TODO: validate this is the right way
        }
        if (l_queue.m_first_deq) {
            if (m_tx->DEBUG_MODE_QUEUE) {
                std::cout << "Queue dequeue - first dequeue" << std::endl;
            }
            l_queue.m_first_deq = false;
            l_queue.m_node_to_deq = m_head;
        } else if (l_queue.m_node_to_deq) {
            l_queue.m_node_to_deq = l_queue.m_node_to_deq.m_next;
        }

        if (l_queue.m_node_to_deq) { // dequeue from the queue
            auto ret = l_queue.m_node_to_deq.m_val;
            q_map.put(this, l_queue);
            unlock(); // TODO: necessary?
            return ret;
        }

        if (m_tx->DEBUG_MODE_QUEUE) {
            std::cout << "Queue dequeue - nodeToDeq is null" << std::endl;
        }

        // there is no node in queue, then try the localQueue
        q_map.put(this, l_queue);
        unlock(); // TODO: necessary?
        return l_queue.dequeue(); // can throw an exception
    }

    void put_local_queue_in_qmap(LocalQueue<val_t> l_queue) {

    }

    val_t dequeueSingleton() {
        while (true) {
            if (tryLock()) {
                if (!m_head) {
                    unlock();
                    throw EmptyQueueException();
                }

                auto node = m_head;
                auto ret = node.m_val;
                m_head = m_head.m_next;
                if (!m_head) {
                    m_tail = NULLOPT;
                } else {
                    m_head.m_prev = NULLOPT;
                }
                m_size--;
                setVersion(m_tx->getVersion());
                setSingleton(true);
                unlock();
                return;
            }
        }
    }

    friend std::ostream& operator<< (std::ostream& stream, const Queue<val_t>& queue) {
        //TODO: this
        auto cur = queue.head;
        while(!cur.is_null()) {
            stream << "," << cur;
            cur = cur->m_next;
        }
        return stream;
    }

    size_t hash() const {
        auto hasher = std::hash<Queue<val_t>*>();
        return hasher(&this);
    }

private:
};
