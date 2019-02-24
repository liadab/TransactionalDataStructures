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

private:
    std::shared_ptr<QNode<val_t>> m_prev;
    std::shared_ptr<QNode<val_t>> m_next;
    Optional<val_t> m_val;
};
