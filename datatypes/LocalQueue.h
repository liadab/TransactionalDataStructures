#pragma once

#include "../nodes/QNode.h"

class EmptyQueueException : public std::exception
{
    const char * what () const throw ()
    {
        return "Queue is empty";
    }
};

template <typename val_t>
class LocalQueue {
public:

    LocalQueue() : m_first_deq(true),
        m_node_to_deq(),
        m_is_locked_by_me(false),
        m_head(QNode<val_t>()),
        m_tail(QNode<val_t>()),
        m_size(0) {
    }

    void enqueue(val_t val) {
        QNode<val_t> node(val, m_tail, NULLOPT);
        m_size++;
        if (!m_tail) {
            m_tail = node;
            m_head = node;

        } else {
            m_tail.m_next = node;
            m_tail = node;
        }
    }

    val_t dequeue(){
        if (!m_head) {
            throw EmptyQueueException();
        }
        auto temp = m_head;
        auto ret = temp.val;
        m_head = m_head.next;
        if (!m_head) {
            m_tail = NULLOPT;
        } else {
            m_head.prev = NULLOPT;
        }
        m_size--;
        return ret;
    }

    bool isEmpty() {
        assert(m_size >= 0 && "LocalQueue size is negative");
        return !m_size;
    }


private:
    bool m_first_deq; // is this the first time of a dequeue?
    QNode<val_t> m_node_to_deq; // the node to dequeue
    bool m_is_locked_by_me; // is queue (not local queue) locked by me
    QNode<val_t> m_head;
    QNode<val_t> m_tail;
    int m_size;
};
