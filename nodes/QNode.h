#pragma once

#include <memory>
#include "../optional.h"

template <typename val_t>
class QNode {
public:
    QNode(val_t val): m_val(val), m_prev(std::shared_ptr<QNode<val_t>>()), m_next(std::shared_ptr<QNode<val_t>>()) {

    }

    QNode(val_t val, std::shared_ptr<QNode<val_t>> &prev, std::shared_ptr<QNode<val_t>> &next):
    m_val(val), m_prev(prev), m_next(next) {

    }

    bool operator==(const QNode<val_t>& other) const {
        return std::tie(m_val, m_prev.get(), m_next.get()) ==
            std::tie(other.m_val, other.m_prev.get(), other.m_next.get());
    }

    bool operator!=(const QNode<val_t>& other) const {
        return std::tie(m_val, m_prev.get(), m_next.get()) !=
            std::tie(other.m_val, other.m_prev.get(), other.m_next.get());
    }

private:
    std::shared_ptr<QNode<val_t>> m_prev;
    std::shared_ptr<QNode<val_t>> m_next;
    Optional<val_t> m_val;
};
