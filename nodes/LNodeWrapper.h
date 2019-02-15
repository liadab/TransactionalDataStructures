#pragma once
#include "LNode.h"

/**
 * this class is wrapping the Lnode so we would have easier control on its memory model.
 * for now its just a shared_ptr wrapper
 */
template <typename key_t, typename val_t>
class LNodeWrapper {
public:
    LNodeWrapper() = default;

    explicit LNodeWrapper(key_t key) {
        m_node = std::make_shared<LNode<key_t, val_t>>(std::move(key));
    }

    LNodeWrapper(key_t key, val_t val) : LNodeWrapper(std::move(key)){
        m_node->m_val = std::move(val);
    }

    bool operator==(const LNodeWrapper<key_t, val_t>& other) {
        return m_node == other.m_node;
    }

    bool operator!=(const LNodeWrapper<key_t, val_t>& other) {
        return m_node != other.m_node;
    }

    bool is_null() {
        return static_cast<bool>(m_node);
    }

    LNode<key_t, val_t>* operator->() {
        return m_node.get();
    }

private:
    std::shared_ptr<LNode<key_t, val_t>> m_node;

};