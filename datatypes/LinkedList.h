#pragma once


#include <memory>
#include <optional>

#include "../nodes/LNode.h"
#include "../nodes/Index.h"
#include "../nodes/LNodeWrapper.h"
#include "../LocalStorage.h"
#include "../WriteElement.h"
#include "dummyIndex.h"
#include "../TX.h"

//pre decle Local storge
//template <typename key_t, typename val_t>
//class LocalStorage;

template <typename key_t, typename val_t>
LNodeWrapper<key_t,val_t> safe_get_next(const LNodeWrapper<key_t,val_t>& n) {
    std::atomic_thread_fence(std::memory_order_acquire);
    auto next = n->m_next;
    std::atomic_thread_fence(std::memory_order_release);
    return next;
}

template <typename key_t, typename val_t>
class LinkedList {
public:
    using node_t = LNodeWrapper<key_t,val_t>;
    using index_t = DummyIndex<key_t, val_t>;

    std::shared_ptr<TX> m_tx;
    node_t head;
    index_t index;

    explicit LinkedList(std::shared_ptr<TX> tx) :
        m_tx(std::move(tx)),
        head(std::numeric_limits<key_t>::min()),
        index(head)
    { }

    node_t getPredSingleton(node_t n) {
        node_t pred = index.getPred(n);
        while (pred->isLockedOrDeleted()) {
            if (pred == head) {
                return head;
            }
            pred = index.getPred(pred);
        }
        return pred;
    }

    std::optional<val_t> getVal(node_t n, LocalStorage<key_t, val_t>& localStorage) {
        auto we_it = localStorage.writeSet.find(n);
        if (we_it != localStorage.writeSet.end()) {
            const auto& we = we_it->second;
            return we.val;
        }
        return n->m_val;
    }

    node_t getPred(node_t n, LocalStorage<key_t, val_t>& localStorage) {
        node_t pred = index.getPred(n);
        while (true) {
            if (pred->isLocked() || pred->getVersion() > localStorage.readVersion) {
                // abort TX
                localStorage.TX = false;
                throw TxAbortException();
            }
            if (pred->isSameVersionAndSingleton(localStorage.readVersion)) {
                // TODO in the case of a thread running singleton and then TX
                // this TX will abort once but for no reason
                m_tx->incrementAndGetVersion();
                localStorage.TX = false;
                throw TxAbortException();
            }
            auto we_it = localStorage.writeSet.find(pred);
            if (we_it != localStorage.writeSet.end()) {
                const auto& we = we_it->second;
                if (we.deleted) {
                    // if you deleted it earlier
                    assert (pred != head);
                    pred = index.getPred(pred);
                    continue;
                }
            }
            if (pred->isDeleted()) {
                assert (pred != head);
                pred = index.getPred(pred);
            } else {
                return pred;
            }
        }
    }

    node_t getNext(node_t n, LocalStorage<key_t, val_t>& localStorage) {
        // first try to read from private write set
        auto we_it = localStorage.writeSet.find(n);
        if (we_it != localStorage.writeSet.end()) {
            const auto& we = we_it->second;
            return we.next;
        }

        // because we don't read next and locked at once,
        // we first see if locked, then read next and then re-check locked
        if (n->isLocked()) {
            // abort TX
            localStorage.TX = false;
            throw TxAbortException();
        }
        auto next = safe_get_next(n);
        if (n->isLocked() || n->getVersion() > localStorage.readVersion) {
            // abort TX
            localStorage.TX = false;
            throw TxAbortException();
        }
        if (n->isSameVersionAndSingleton(localStorage.readVersion)) {
            m_tx->incrementAndGetVersion();
            localStorage.TX = false;
            throw TxAbortException();
        }
        return next;
    }

    //find a node if found return true, pred and the node otherwise false with pred as the one that should be bfore the node
    std::tuple<bool, node_t, node_t> find_node_singelton(LocalStorage<key_t, val_t>& localStorage, node_t n) {
        auto key = n->m_key;
        while (true) {
            bool startOver = false;
            auto pred = getPredSingleton(n);
            if (pred->isLocked()) {
                continue;
            }
            auto next = safe_get_next(pred);
            if (pred->isLockedOrDeleted()) {
                continue;
            }
            while (!next.is_null()) {
                if (next->isLockedOrDeleted()) {
                    // when we encounter a locked node while traversing the list
                    // we have to start over
                    startOver = true;
                    break;
                }

                if (next->m_key == key) {
                    // the key exists, change to new value
                    auto node = pred->m_next;
                    return std::make_tuple(true, pred, node);
                } else if (next->m_key > key) {
                    return std::make_tuple(false, pred, next);
                }
                // next is still strictly less than key
                if (next->isLocked() || next != pred->m_next) {
                    startOver = true;
                    break;
                }
                std::atomic_thread_fence(std::memory_order_acquire);
                pred = pred->m_next;
                if (pred.is_null()) {
                    // next was not null but now perd->m_next is null
                    // to prevent null exception later
                    startOver = true;
                    break;
                }
                next = pred->m_next;
                std::atomic_thread_fence(std::memory_order_acquire);
                if (pred->isLockedOrDeleted()) {
                    startOver = true;
                    break;
                }
            }

            if (startOver) { continue; }
            return std::make_tuple(false, pred, LNodeWrapper<key_t,val_t>());
        }
    }

    //find a node if found return true, pred and the node otherwise false
    std::tuple<bool, node_t, node_t> find_node(LocalStorage<key_t, val_t>& localStorage, node_t n) {
        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);
        bool found = false;

        while (next.is_null()) {
            if (next->m_key == n->m_key) {
                return std::make_tuple(true, pred, next);
            } else if (next->m_key > n->m_key) {
                return std::make_tuple(false, pred, next);
            } else {
                pred = next;
                next = getNext(pred, localStorage);
            }
        }
        return std::make_tuple(false, pred, next);
    }

    std::optional<val_t> putSingleton(key_t key, val_t val) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        node_t n(std::move(key), std::move(val));
        while (true) {
            auto [found, pred, next] = find_node_singelton(localStorage, n);
            if (found) {
                // the key exists, change to new value
                auto node = pred->m_next;
                if (node->tryLock()) {
                    if (next->m_key != key || node != next || node->isDeleted()) {
                        node->unlock();
                        continue;
                    }
                    auto ret = node->m_val;
                    node->m_val = val;
                    node->setSingleton(true);
                    node->setVersion(m_tx->getVersion());
                    node->unlock();
                    return ret; // return previous value associated with key
                } else {
                    continue;
                }
            } else if (!next.is_null()) {
                // key doesn't exist, perform insert
                if (pred->tryLock()) {
                    if (pred->isDeleted() || next != pred->m_next) {
                        pred->unlock();
                        continue;
                    }
                    n->m_next = pred->m_next;
                    pred->m_next = n;
                    n->setVersionAndSingletonNoLockAssert(m_tx->getVersion(), true);
                    pred->unlock();
                    index.add(n);
                    return std::nullopt;
                } else {
                    continue;
                }
            } else {
                // all are strictly less than key
                // put at end
                if (pred->tryLock()) {
                    if (pred->isDeleted() || pred->m_next.is_not_null()) {
                        pred->unlock();
                        continue;
                    }
                    pred->m_next = n;
                    pred->unlock();
                    index.add(n);
                    return std::nullopt;
                }
            }
        }
    }


    // Associates the specified value with the specified key in this map.
    // If the map previously contained a mapping for the key, the old value
    // is replaced.
    // returns the previous value associated with key, or null if there was no
    // mapping for key. (A null return can also indicate that the map previously
    // associated null with key, if the implementation supports null values.)
    // @throws NullPointerException if the specified key or value is null
    std::optional<val_t> put(key_t key, val_t val) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();

        // SINGLETON
        if (!localStorage.TX) {
            return putSingleton(key, val);
        }

        // TX

        localStorage.readOnly = false;
        node_t n(std::move(key), std::move(val));
        auto [found, pred, next] = find_node(localStorage, n);

        if (found) {
            auto we_it = localStorage.writeSet.find(pred);
            if (we_it != localStorage.writeSet.end()) {
                const auto& we = we_it->second;
                localStorage.putIntoWriteSet(next, we.next, val, we.deleted);
            } else {
                localStorage.putIntoWriteSet(next, next->m_next, val, false);
            }
            // add to read set
            localStorage.readSet.emplace(next);
            if (m_tx->DEBUG_MODE_LL) {
                std::cout << "put key " << key << ":" << std::endl;
                //printWriteSet();
            }
            return next->m_val;
        }

        // not found
        n->m_next = next;
        localStorage.putIntoWriteSet(pred, n, getVal(pred, localStorage), false);
        //TODO make shared from this
        localStorage.addToIndexAdd(this, n);

        // add to read set
        localStorage.readSet.emplace(pred);

        if (m_tx->DEBUG_MODE_LL) {
            std::cout << "put key " << key  << ":" << std::endl;
           // printWriteSet();
        }

        return std::nullopt;
    }

    std::optional<val_t> putIfAbsentSingleton(key_t key, val_t val) {
        node_t n(std::move(key), std::move(val));
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        while (true) {
            auto[found, pred, next] = find_node_singelton(localStorage, n);
            if (found) {
                // the key exists, return value
                auto node = pred->m_next;
                if (next->key != key || node != next || node->isLockedOrDeleted()) {
                    continue;
                }
                // return previous value associated with key
                return node.val;
            } else if (!next.is_null()) {
                // key doesn't exist, perform insert
                if (pred->tryLock()) {
                    if (pred->isDeleted() || next != pred->m_next) {
                        pred->unlock();
                        continue;
                    }
                    n->m_next = pred->m_next;
                    pred->m_next = n;
                    n->setVersionAndSingletonNoLockAssert(m_tx->getVersion(), true);
                    pred->unlock();
                    index.add(n);
                    return std::nullopt;
                } else {
                    continue;
                }
            } else {
                // all are strictly less than key
                // put at end
                if (pred->tryLock()) {
                    if (pred->isDeleted() || pred->m_next.is_not_null()) {
                        pred->unlock();
                        continue;
                    }

                    pred->m_next = n;
                    pred->unlock();
                    index.add(n);
                    return std::nullopt;
                }
            }
        }
    }

    // If the specified key is not already associated with a value,
    // associate it with the given value.
    // Returns the previous value associated with the specified key,
    // or null if there was no mapping for the key.
    // (A null return can also indicate that the map previously associated
    // null with the key, if the implementation supports null values.)
    // @throws NullPointerException if the specified key or value is null
    std::optional<val_t> putIfAbsent(key_t key, val_t val) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();

        // SINGLETON
        if (!localStorage.TX) {
            return putIfAbsentSingleton(key, val);
        }

        // TX
        localStorage.readOnly = false;
        node_t n(std::move(key), std::move(val));
        auto [found, pred, next] = find_node(localStorage, n);

        if (found) {
            // the key exists, return value
            localStorage.readSet.emplace(next); // add to read set
            return next.val;
        }

        // not found
        n->m_next = next;
        localStorage.putIntoWriteSet(pred, n, getVal(pred, localStorage), false);
        localStorage.addToIndexAdd(this, n);
        localStorage.readSet.emplace(pred); // add to read set
        return std::nullopt;
    }

    std::optional<val_t> removeSingleton(key_t key) {
        node_t n(std::move(key));
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        while (true) {
            auto[found, pred, next] = find_node_singelton(localStorage, n);
            if (!found) {
                return std::nullopt;
            }
            //found key trying to remove
            // the key exists
            if (m_tx->DEBUG_MODE_LL) {
                std::cout << ("removeSingleton: the key exists " + key) << std::endl;
            }
            if (pred->tryLock()) {
                if (m_tx->DEBUG_MODE_LL) {
                    std::cout << ("removeSingleton: pred was locked of key " + key) << std::endl;
                }
                if (pred->isDeleted() || next != pred->m_next) {
                    pred->unlock();
                    continue;
                }
                node_t toRemove;
                std::optional<val_t> valToRet;
                if (next->tryLock()) {
                    toRemove = next;
                    valToRet = toRemove->m_val;
                    toRemove->m_val = std::nullopt; // for Index
                    pred->m_next = pred->m_next->m_next;
                    auto ver = m_tx->getVersion();
                    toRemove->setVersionAndDeletedAndSingleton(ver, true, true);
                    pred->setVersionAndSingleton(ver, true);
                    if (m_tx->DEBUG_MODE_LL) {
                        std::cout << ("removeSingleton: removed key " + key) << std::endl;
                    }
                } else {
                    pred->unlock();
                    continue;
                }
                toRemove->unlock();
                pred->unlock();
                index.remove(toRemove);
                return valToRet;
            } else {
                if (m_tx->DEBUG_MODE_LL) {
                    std::cout << ("removeSingleton: the key exists, couldn't lock " + key) << std::endl;
                }
                continue;
            }
        }
    }


    // Removes the mapping for a key from this map if it is present
    // Returns the value to which this map previously associated the key,
    // or null if the map contained no mapping for the key.
    // @throws NullPointerException if the specified key is null
    std::optional<val_t> remove(key_t key) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        // SINGLETON
        if (!localStorage.TX) {
            return removeSingleton(key);
        }

        // TX

        localStorage.readOnly = false;
        node_t n(std::move(key));
        auto [found, pred, next] = find_node(localStorage, n);
        // add to read set
        localStorage.readSet.emplace(pred);


        if (found) {
            localStorage.putIntoWriteSet(pred, getNext(next, localStorage), getVal(pred, localStorage), false);
            localStorage.putIntoWriteSet(next, node_t(), getVal(next, localStorage), true);
            // add to read set
            localStorage.readSet.emplace(next);
            localStorage.addToIndexRemove(this, next);
            auto we_it = localStorage.writeSet.find(next);
            if (we_it != localStorage.writeSet.end()) {
                const auto& we = we_it->second;
                return we.val;
            }
            return next->m_val;
        }
        //not found
        return std::nullopt;
    }

    bool containsKeySingleton(key_t key) {
        node_t n(std::move(key));
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        //TODO maybe only get pred is more efficent
        auto[found, pred, next] = find_node_singelton(localStorage, n);
        return found;
    }

    bool containsKey(key_t key) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        // SINGLETON
        if (!localStorage.TX) {
            return containsKeySingleton(key);
        }
        // TX
        node_t n(std::move(key));
        auto [found, pred, next] = find_node(localStorage, n);
        return found;
    }

    std::optional<val_t> getSingleton(key_t key) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        node_t n(std::move(key));
        //TODO maybe only get pred is more efficent
        auto[found, pred, next] = find_node_singelton(localStorage, n);
        if(found) {
            return next->m_val;
        }
        return std::nullopt;
    }

    std::optional<val_t> get(key_t key) {
        auto& localStorage = m_tx->get_local_storge<key_t, val_t>();
        // SINGLETON
        if (!localStorage.TX) {
            return getSingleton(key);
        }

        // TX
        node_t n(std::move(key));
        auto [found, pred, next] = find_node(localStorage, n);
        if (m_tx->DEBUG_MODE_LL) {
            std::cout << "get key " << key << ":" << std::endl;
            std::cout << "pred is " << pred->m_key << std::endl;
//            printWriteSet();
        }

        // add to read set
        localStorage.readSet.emplace(pred);
        if(found) {
            assert (next->m_key == key);
            return next->m_val;
        }
        return std::nullopt;
    }

    friend std::ostream& operator<< (std::ostream& stream, const LinkedList<key_t, val_t>& list) {
        auto cur = list.head;
        while(!cur.is_null()) {
            stream << "," << cur;
            cur = cur->m_next;
        }
        return stream;
    }
};


