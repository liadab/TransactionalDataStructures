#pragma once

#include "nodes/LNode.h"

template <typename T>
std::ostream& operator<< (std::ostream& stream, const std::optional<T>& v) {
    if(!v) {
        stream << "None";
    } else {
        stream << *v;
    }
    return stream;
}

template <typename key_t, typename val_t>
class WriteElement {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    WriteElement() : deleted(false) {}
    node_t next;
    std::optional<val_t> val; // used when changing value with put
    bool deleted;

    friend std::ostream& operator<< (std::ostream& stream, const WriteElement<key_t, val_t>& we) {
        stream << "val: " << we.val << " d?" << we.deleted << " next: " << we.next;
        return stream;
    }
};