#pragma once

#include "nodes/LNode.h"

template <typename key_t, typename val_t>
class WriteElement {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    WriteElement() : deleted(false) {}
    node_t next;
    Optional<val_t> val; // used when changing value with put
    bool deleted;

    friend std::ostream& operator<< (std::ostream& stream, const WriteElement<key_t, val_t>& we) {
        stream << "val: " << we.val << " d?" << we.deleted << " next: " << we.next;
        return stream;
    }
};