#pragma once

template <typename key_t, typename val_t>
class DummyIndex {
public:
    using node_t = LNodeWrapper<key_t,val_t>;
    DummyIndex(node_t head_node) {}

    void add(node_t node_to_add) {}
    void remove(LNode<key_t, val_t> node) {}
    node_t getPred(node_t node) {}
};