#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ostream>

#include "nodes/LNode.h"
#include "WriteElement.h"

template <typename key_t, typename val_t>
class LinkedList;

template <typename key_t, typename val_t>
class LocalStorage {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    //TODO add when queue is implmentd
    //std::map<Queue, LocalQueue> queueMap = new HashMap<Queue, LocalQueue>();
    std::unordered_map<node_t, WriteElement<key_t, val_t>> writeSet;
    std::unordered_set<node_t> readSet;
    std::unordered_map<LinkedList<key_t, val_t>*, std::vector<node_t>> indexAdd;
    std::unordered_map<LinkedList<key_t, val_t>*, std::vector<node_t>> indexRemove;

    void putIntoWriteSet(node_t node, node_t next, Optional<val_t> val, bool deleted) {
        WriteElement<key_t, val_t> we;
        we.next = next;
        we.deleted = deleted;
        we.val = val;
        writeSet[node] = we;
    }

    void addToIndexAdd(LinkedList<key_t, val_t>* list, node_t node) {
        auto nodes_it = indexAdd.find(list);
        if(indexAdd.count(list) == 0) {
            indexAdd[list] = std::vector<node_t>();
        }
        auto& nodes = indexAdd[list];
        nodes.push_back(std::move(node));
        //indexAdd.put(list, nodes);
    }

    void addToIndexRemove(LinkedList<key_t, val_t>* list, node_t node) {
        auto nodes_it = indexRemove.find(list);
        if(indexRemove.count(list) == 0) {
            indexRemove[list] = std::vector<node_t>();
        }
        auto& nodes = indexRemove[list];
        nodes.push_back(std::move(node));
        //indexRemove.put(list, nodes);
    }

};

