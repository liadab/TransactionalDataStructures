#pragma once

#include "nodes/LNode.h"

template <typename key_t, typename val_t>
class WriteElement {
public:
    WriteElement() : deleted(false) {}
    std::unique_ptr<LNode<key_t,val_t>> next;
    std::unique_ptr<val_t> val; // used when changing value with put
    bool deleted;
};