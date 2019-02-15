#pragma once

#include "nodes/LNode.h"

template <typename key_t, typename val_t>
class WriteElement {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    WriteElement() : deleted(false) {}
    node_t next;
    val_t val; // used when changing value with put
    bool deleted;
};