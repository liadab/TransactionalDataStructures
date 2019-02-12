#pragma once


#include <memory>
#include <optional>

#include "../nodes/LNode.h"
#include "../nodes/Index.h"

template <typename key_t, typename val_t>
void safe_assign_next(std::shared_ptr<LNode<key_t, val_t>> next, std::shared_ptr<LNode<key_t, val_t>> n) {
    std::atomic_thread_fence(std::memory_order_acquire);
    std::shared_ptr<LNode<key_t, val_t>> next = n.next;
    std::atomic_thread_fence(std::memory_order_release);
}

template <typename key_t, typename val_t>
class LinkedList {
public:
    std::shared_ptr<LNode<key_t, val_t>> head;
    Index<key_t, val_t> index;

    LinkedList() :
        head(std::make_shared<LNode<key_t, val_t>>()),
        index(head)
    {
        head->m_key = std::make_unique<key_t>(std::numeric_limits<key_t>::min());
    }

    std::shared_ptr<LNode<key_t, val_t>> getPredSingleton(std::shared_ptr<LNode<key_t, val_t>> n) {
        std::shared_ptr<LNode<key_t, val_t>> pred = index.getPred(n);
        while (pred.isLockedOrDeleted()) {
            if (pred == head) {
                return head;
            }
            pred = index.getPred(pred);
        }
        return pred;
    }

    val_t getVal(LNode<key_t, val_t> n, LocalStorage localStorage) {
        std::optional<WriteElement> we = localStorage.writeSet.get(n);
        if (we) {
            return we.val;
        }
        return n.val;
    }

    std::shared_ptr<LNode<key_t, val_t>> getPred(LNode<key_t, val_t> n, LocalStorage localStorage) {
        LNode<key_t, val_t> pred = index.getPred(n);
        while (true) {
            if (pred.isLocked() || pred.getVersion() > localStorage.readVersion) {
                // abort TX
                localStorage.TX = false;
                
                throw TxAbortException();
            }
            if (pred.isSameVersionAndSingleton(localStorage.readVersion)) {
                // TODO in the case of a thread running singleton and then TX
                // this TX will abort once but for no reason
                TX.incrementAndGetVersion();
                localStorage.TX = false;
                throw TxAbortException();
            }
            WriteElement we = localStorage.writeSet.get(pred);
            if (we != null) {
                if (we.deleted) {
                    // if you deleted it earlier
                    assert (pred != head);
                    pred = index.getPred(pred);
                    continue;
                }
            }
            if (pred.isDeleted()) {
                assert (pred != head);
                pred = index.getPred(pred);
            } else {
                return pred;
            }
        }
    }

    std::shared_ptr<LNode<key_t, val_t>> getNext(std::shared_ptr<LNode<key_t, val_t>> n, LocalStorage& localStorage) {
        // first try to read from private write set
        WriteElement we = localStorage.writeSet.get(n);
        if (we != null) {
            return we.next;
        }

        // because we don't read next and locked at once,
        // we first see if locked, then read next and then re-check locked
        if (n.isLocked()) {
            // abort TX
            localStorage.TX = false;
            throw TxAbortException();
        }
        safe_assign_next(next, n);
        if (n.isLocked() || n.getVersion() > localStorage.readVersion) {
            // abort TX
            localStorage.TX = false;
            throw TxAbortException();
        }
        if (n.isSameVersionAndSingleton(localStorage.readVersion)) {
            TX.incrementAndGetVersion();
            localStorage.TX = false;
            throw TxAbortException();
        }
        return next;
    }

    std::optional<val_t> putSingleton(key_t key, val_t val) {
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n->key = key;
        n->val = val;

        std::shared_ptr<LNode<key_t, val_t>> pred;
        std::shared_ptr<LNode<key_t, val_t>> next;

        while (true) {
            boolean startOver = false;
            pred = getPredSingleton(n);
            if (pred->isLocked()) {
                continue;
            }
            safe_assign_next(next, pred);
            if (pred.isLockedOrDeleted()) {
                continue;
            }

            while (next != null) {
                if (next.isLockedOrDeleted()) {
                    // when we encounter a locked node while traversing the list
                    // we have to start over
                    startOver = true;
                    break;
                }

                if (next.key == key) {
                    // the key exists, change to new value
                    auto node = pred.next;
                    if (node.tryLock()) {

                        if (next.key != key || node != next || node.isDeleted()) {
                            node.unlock();
                            startOver = true;
                            break;
                        }
                        auto ret = node.val;
                        node.val = val;
                        node.setSingleton(true);
                        node.setVersion(TX.getVersion());
                        node.unlock();
                        return ret; // return previous value associated with key
                    } else {
                        startOver = true;
                        break;
                    }
                } else if (next.key > key) {
                    // key doesn't exist, perform insert
                    if (pred.tryLock()) {

                        if (pred.isDeleted() || next != pred.next) {
                            pred.unlock();
                            startOver = true;
                            break;
                        }

                        n.next = pred.next;
                        pred.next = n;
                        n.setVersionAndSingletonNoLockAssert(TX.getVersion(), true);
                        pred.unlock();
                        index.add(n);
                        return null;
                    } else {
                        startOver = true;
                        break;
                    }
                }
                // next is still strictly less than key
                if (next.isLocked() || next != pred.next) {
                    startOver = true;
                    break;
                }
                std::atomic_thread_fence(std::memory_order_acquire);
                pred = pred.next;
                if (pred == null) {
                    // next was not null but now perd.next is null
                    // to prevent null exception later
                    startOver = true;
                    break;
                }
                next = pred.next;
                std::atomic_thread_fence(std::memory_order_acquire);
                if (pred.isLockedOrDeleted()) {
                    startOver = true;
                    break;
                }
            }

            if (startOver) {
                continue;
            }

            // all are strictly less than key
            // put at end
            if (pred.tryLock()) {

                if (pred.isDeleted() || pred.next != null) {
                    pred.unlock();
                    continue;
                }

                pred.next = n;
                pred.unlock();
                index.add(n);
                return null;
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
        LocalStorage localStorage = TX.lStorage.get();

        // SINGLETON
        if (!localStorage.TX) {
            return putSingleton(key, val);
        }

        // TX

        localStorage.readOnly = false;
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = val;

        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);
        bool found = false;

        while (next != null) {
            if (next.key == key) {
                found = true;
                break;
            } else if (next.key > key) {
                break;
            } else {
                pred = next;
                next = getNext(pred, localStorage);
            }
        }

        if (found) {
            std::optional<WriteElement> we = localStorage.writeSet.get(next);
            if (we != null) {
                // if it is already in write set then just change val
                localStorage.putIntoWriteSet(next, we->next, val, we->deleted);
            } else {
                localStorage.putIntoWriteSet(next, next.next, val, false);
            }
            // add to read set
            localStorage.readSet.add(next);
            if (TX.DEBUG_MODE_LL) {
                System.out.println("put key " + key + ":");
                printWriteSet();
            }
            return next.val;
        }

        // not found
        n.next = next;
        localStorage.putIntoWriteSet(pred, n, getVal(pred, localStorage), false);
        localStorage.addToIndexAdd(this, n);

        // add to read set
        localStorage.readSet.add(pred);

        if (TX.DEBUG_MODE_LL) {
            System.out.println("put key " + key + ":");
            printWriteSet();
        }

        return null;
    }

    std::optional<val_t> putIfAbsentSingleton(key_t key, val_t val) {
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = val;

        std::shared_ptr<LNode<key_t, val_t>> pred;
        std::shared_ptr<LNode<key_t, val_t>> next;

        while (true) {

            bool startOver = false;

            pred = getPredSingleton(n);
            if (pred.isLocked()) {
                continue;
            }
            safe_assign_next(next, pred);
            if (pred.isLockedOrDeleted()) {
                continue;
            }

            while (next != null) {

                if (next.isLockedOrDeleted()) {
                    // when we encounter a locked node while traversing the list
                    // we have to start over
                    startOver = true;
                    break;
                }

                if (next.key == key) {
                    // the key exists, return value
                    auto node = pred.next;
                    if (next.key != key || node != next || node.isLockedOrDeleted()) {
//						node.unlock();
                        startOver = true;
                        break;
                    }
                    // return previous value associated with key
                    return node.val;
                } else if (next.key > key) {
                    // key doesn't exist, perform insert
                    if (pred.tryLock()) {
                        if (pred.isDeleted() || next != pred.next) {
                            pred.unlock();
                            startOver = true;
                            break;
                        }

                        n.next = pred.next;
                        pred.next = n;
                        n.setVersionAndSingletonNoLockAssert(TX.getVersion(), true);
                        pred.unlock();
                        index.add(n);
                        return null;
                    } else {
                        startOver = true;
                        break;
                    }
                }
                // next is still strictly less than key
                if (next.isLocked() || next != pred.next) {
                    startOver = true;
                    break;
                }
                unsafe.loadFence();
                pred = pred.next;
                if (pred == null) {
                    // next was not null but now perd.next is null
                    // to prevent null exception later
                    startOver = true;
                    break;
                }
                next = pred.next;
                std::atomic_thread_fence(std::memory_order_acquire);
                if (pred.isLockedOrDeleted()) {
                    startOver = true;
                    break;
                }
            }

            if (startOver) {
                continue;
            }

            // all are strictly less than key
            // put at end
            if (pred.tryLock()) {
                if (pred.isDeleted() || pred.next != null) {
                    pred.unlock();
                    continue;
                }

                pred.next = n;
                pred.unlock();
                index.add(n);
                return null;
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
        LocalStorage localStorage = TX.lStorage.get();

        // SINGLETON
        if (!localStorage.TX) {
            return putIfAbsentSingleton(key, val);
        }

        // TX
        localStorage.readOnly = false;
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = val;

        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);
        bool found = false;

        while (next != null) {
            if (next.key == key) {
                found = true;
                break;
            } else if (next.key > key) {
                break;
            } else {
                pred = next;
                next = getNext(pred, localStorage);
            }
        }

        if (found) {
            // the key exists, return value
            localStorage.readSet.add(next); // add to read set
            return next.val;
        }

        // not found
        n.next = next;
        localStorage.putIntoWriteSet(pred, n, getVal(pred, localStorage), false);
        localStorage.addToIndexAdd(this, n);
        localStorage.readSet.add(pred); // add to read set

        return null;
    }

    std::optional<val_t> removeSingleton(key_t key) {
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;

        std::shared_ptr<LNode<key_t, val_t>> pred;
        std::shared_ptr<LNode<key_t, val_t>> next;

        while (true) {

            bool startOver = false;

            pred = getPredSingleton(n);
            if (pred.isLocked()) {
                continue;
            }
            safe_assign_next(next, pred);
            if (pred.isLockedOrDeleted()) {
                continue;
            }

            while (next != null) {

                if (next.isLockedOrDeleted()) {
                    // when we encounter a locked node while traversing the list
                    // we have to start over
                    startOver = true;
                    break;
                }

                if (next.key < key) {
                    if (next.isLocked() || next != pred.next) {
                        startOver = true;
                        break;
                    }
                    std::atomic_thread_fence(std::memory_order_acquire);
                    pred = pred.next;
                    if (pred == null) {
                        // next was not null but now perd.next is null
                        // to prevent null exception later
                        startOver = true;
                        break;
                    }
                    next = pred.next;
                    std::atomic_thread_fence(std::memory_order_release);
                    if (pred.isLockedOrDeleted()) {
                        startOver = true;
                        break;
                    }
                } else if (next.key > key) {
                    if (next != pred.next) {
                        startOver = true;
                        break;
                    }
                    // key does not exist
                    return null;
                } else {
                    // the key exists
                    if (TX.DEBUG_MODE_LL) {
                        System.out.println("removeSingleton: the key exists " + key);
                    }
                    if (pred.tryLock()) {
                        if (TX.DEBUG_MODE_LL) {
                            System.out.println("removeSingleton: pred was locked of key " + key);
                        }
                        if (pred.isDeleted() || next != pred.next) {
                            pred.unlock();
                            startOver = true;
                            break;
                        }
                        std::shared_ptr<LNode<key_t, val_t>> toRemove;
                        Object valToRet;
                        if (next.tryLock()) {
                            toRemove = next;
                            valToRet = toRemove.val;
                            toRemove.val = null; // for Index
                            pred.next = pred.next.next;
                            auto ver = TX.getVersion();
                            toRemove.setVersionAndDeletedAndSingleton(ver, true, true);
                            pred.setVersionAndSingleton(ver, true);
                            if (TX.DEBUG_MODE_LL) {
                                System.out.println("removeSingleton: removed key " + key);
                            }
                        } else {
                            pred.unlock();
                            startOver = true;
                            break;
                        }
                        toRemove.unlock();
                        pred.unlock();
                        index.remove(toRemove);
                        return valToRet;
                    } else {
                        if (TX.DEBUG_MODE_LL) {
                            System.out.println("removeSingleton: the key exists, couldn't lock " + key);
                        }
                        startOver = true;
                        break;
                    }
                }

            }

            if (startOver) {
                continue;
            }

            return null;
        }

    }

    // Removes the mapping for a key from this map if it is present
    // Returns the value to which this map previously associated the key,
    // or null if the map contained no mapping for the key.
    // @throws NullPointerException if the specified key is null
    std::optional<val_t> remove(key_t key) {
        LocalStorage localStorage = TX.lStorage.get();

        // SINGLETON
        if (!localStorage.TX) {
            return removeSingleton(key);
        }

        // TX

        localStorage.readOnly = false;

        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = std::none;

        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);
        bool found = false;

        while (next != null) {
            if (next.key == key) {
                found = true;
                break;
            } else if (next.key > key) {
                break;
            } else {
                pred = next;
                next = getNext(pred, localStorage);
            }
        }

        if (found) {
            localStorage.putIntoWriteSet(pred, getNext(next, localStorage), getVal(pred, localStorage), false);
            localStorage.putIntoWriteSet(next, null, getVal(next, localStorage), true);
            // add to read set
            localStorage.readSet.add(next);
            localStorage.addToIndexRemove(this, next);
        }

        // add to read set
        localStorage.readSet.add(pred);

        if (!found) {
            return null;
        }
        WriteElement we = localStorage.writeSet.get(next);
        if (we != null) {
            return we.val;
        }
        return next.val;
    }

    bool containsKeySingleton(key_t key) {
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;

        std::shared_ptr<LNode<key_t, val_t>> pred = null;
        std::shared_ptr<LNode<key_t, val_t>> next;

        bool startOver = true;

        while (true) {

            if (startOver) {
                pred = getPredSingleton(n);
            } else {
                pred = pred.next;
                if (pred == null) {
                    // next was not null but now perd.next is null
                    // to prevent null exception later
                    startOver = true;
                    continue;
                }
            }

            startOver = false;

            if (pred.isLocked()) {
                startOver = true;
                continue;
            }
            safe_assign_next(next, pred);
            if (pred.isLockedOrDeleted()) {
                startOver = true;
                continue;
            }

            if (next == null || next.key > key) {
                // key does not exist
                return false;
            } else if (next.key == key) {
                return true;
            } else {
                assert (next.key < key);
                if (next != pred.next) {
                    startOver = true;
                }
            }

        }

    }

    bool containsKey(key_t key) {
        LocalStorage localStorage = TX.lStorage.get();

        // SINGLETON
        if (!localStorage.TX) {
            return containsKeySingleton(key);
        }

        // TX
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = null;

        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);

        while (next != null && next.key < key) {
            pred = next;
            next = getNext(pred, localStorage);
        }

        // add to read set
        localStorage.readSet.add(pred);

        if (next == null || next.key > key) {
            return false;
        } else {
            assert (next.key == key);
            return true;
        }

    }

    std::optional<val_t> getSingleton(key_t key) {
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;

        std::shared_ptr<LNode<key_t, val_t>> pred = null;
        std::shared_ptr<LNode<key_t, val_t>> next;

        boolean startOver = true;

        while (true) {

            if (startOver) {
                pred = getPredSingleton(n);
            } else {
                pred = pred.next;
                if (pred == null) {
                    // next was not null but now perd.next is null
                    // to prevent null exception later
                    startOver = true;
                    continue;
                }
            }

            startOver = false;

            if (pred.isLocked()) {
                startOver = true;
                continue;
            }
            safe_assign_next(next, pred);
            if (pred.isLockedOrDeleted()) {
                startOver = true;
                continue;
            }

            if (next == null || next.key > key) {
                // key does not exist
                return null;
            } else if (next.key == key) {
                return next.val;
            } else {
                assert (next.key < key);
                if (next != pred.next) {
                    startOver = true;
                }
            }

        }

    }

    std::optional<val_t> get(key_t key) {

        LocalStorage localStorage = TX.lStorage.get();

        // SINGLETON
        if (!localStorage.TX) {
            return getSingleton(key);
        }

        // TX
        std::shared_ptr<LNode<key_t, val_t>> n = std::make_shared<LNode<key_t, val_t>>();
        n.key = key;
        n.val = null;

        auto pred = getPred(n, localStorage);
        auto next = getNext(pred, localStorage);

        while (next != null && next.key < key) {
            pred = next;
            next = getNext(pred, localStorage);
        }

        if (TX.DEBUG_MODE_LL) {
            System.out.println("get key " + key + ":");
            System.out.println("pred is " + pred.key);
            printWriteSet();
        }

        // add to read set
        localStorage.readSet.add(pred);

        if (next == null || next.key > key) {
            return null;
        } else {
            assert (next.key == key);
            return getVal(next, localStorage);
        }
    }

};


