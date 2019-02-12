#pragma once

#include <unordered_map>
#include <unordered_set>

#include "nodes/LNode.h"
#include "WriteElement.h"
#include "datatypes/LinkedList.h"

template <typename key_t, typename val_t>
class LocalStorage {
 size_t readVersion = 0L;
 size_t writeVersion = 0L; // for debug
 bool TX = false;
 bool readOnly = true;
 //TODO add when queue is implmentd
 //std::map<Queue, LocalQueue> queueMap = new HashMap<Queue, LocalQueue>();
    std::unordered_map<std::shared_ptr<LNode<key_t, val_t>>, WriteElement<key_t, val_t>> writeSet;
    std::unordered_set<std::shared_ptr<LNode<key_t, val_t>>> readSet;
    std::unordered_map<LinkedList<key_t, val_t>, std::vector<std::shared_ptr<LNode<key_t, val_t>>>> indexAdd;
    std::unordered_map<LinkedList<key_t, val_t>, std::vector<std::shared_ptr<LNode<key_t, val_t>>>> indexRemove;

  void putIntoWriteSet(std::shared_ptr<LNode<key_t, val_t>> node, std::shared_ptr<LNode<key_t, val_t>> next, val_t val, bool deleted) {
        WriteElement<key_t, val_t>> we;
        we.next = next;
        we.deleted = deleted;
        we.val = val;
        writeSet.put(node, we);
    }

 void addToIndexAdd(LinkedList<key_t, val_t> list, std::shared_ptr<LNode<key_t, val_t>> node) {
      //TODO i assume that get build an empty vector if list is not found
        std::vector<std::shared_ptr<LNode<key_t, val_t>>> nodes = indexAdd.get(list);
        nodes.add(node);
        indexAdd.put(list, nodes);
    }

    void addToIndexRemove(LinkedList<key_t, val_t> list, std::shared_ptr<LNode<key_t, val_t>> node) {
        std::vector<LNode> nodes = indexRemove.get(list);
        nodes.add(node);
        indexRemove.put(list, nodes);
    }
};

