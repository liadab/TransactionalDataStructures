#pragma once


#include "LNode.h"

template <typename key_t, typename val_t>
class LNodeWrapper {
public:
    LNodeWrapper() = default;

    LNodeWrapper(key_t key) {
        m_node = std::make_shared<LNode<key_t, val_t>>();
        m_node->m_key = key;
    }

    LNode<key_t, val_t>* operator->() {
        return m_node.get();
    }

    bool is_null() {
        return m_node == null;
    }

private:
    std::shared_ptr<LNode<key_t, val_t>> m_node;

};


#endif //TRANSACTIONALDATASTRUCTURES_LNODEWRAPPER_H
