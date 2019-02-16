#pragma once

template <typename key_t, typename val_t>
class DummyIndex {
public:

    using node_t = LNodeWrapper<key_t,val_t>;
    DummyIndex(node_t head_node) : m_head(head_node) {}

    void add(node_t node_to_add) {}
    void remove(node_t node) {}
    node_t getPred(node_t node) { return m_head; }

    node_t m_head;
};