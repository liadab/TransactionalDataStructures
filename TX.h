#pragma once

#include <atomic>
#include "LocalTransaction.h"
class TxAbortException : public std::exception
{
    const char * what () const throw ()
    {
        return "TX abort";
    }
};

class TX {
public:
    std::atomic<uint64_t> gvc;

    static constexpr bool DEBUG_MODE_LL = false;
    static constexpr bool DEBUG_MODE_QUEUE = false;
    static constexpr bool DEBUG_MODE_TX = false;
    static constexpr bool DEBUG_MODE_VERSION = false;

    TX() : gvc(0) {}

    uint64_t getVersion() const {
        return gvc;
    }

    uint64_t incrementAndGetVersion() {
        return ++gvc;
    }

    template <typename key_t, typename val_t>
    LocalStorage<key_t, val_t>& get_local_storge() const {
        static thread_local  LocalStorage<key_t, val_t> lStorage;
        return lStorage;
    }

    LocalTransaction& get_local_transaction() const {
        static thread_local  LocalTransaction transaction;
        return transaction;
    }

    void TXbegin() {
        if (DEBUG_MODE_TX) {
            std::cout << "begin transction" << std::endl;
        }

        auto& local_transaction = get_local_transaction();
        local_transaction.TX = true;
        local_transaction.readVersion = getVersion();
    }

    template <typename key_t, typename val_t>
    bool TXend() {
        using node_t = LNodeWrapper<key_t,val_t>;

        if (DEBUG_MODE_TX) {
            std::cout << "TXend" << std::endl;
        }

        bool abort = false;

        auto& localStorage = get_local_storge<key_t, val_t>();
        auto& local_transaction = get_local_transaction();

        if (!local_transaction.TX) {
            abort = true;
        }

        // locking write set
        auto& writeSet = localStorage.writeSet;
        std::unordered_set<node_t> lockedLNodes;
        if (!abort) {
            for (auto node_and_we : writeSet) {
                node_t node = node_and_we.first;
                if (!node->tryLock()) {
                    abort = true;
                    break;
                }
                lockedLNodes.emplace(std::move(node));
            }
        }

        // locking queues TODO implment
//        HashMap<Queue, LocalQueue> qMap = localStorage.queueMap;
//
//        if (!abort) {
//
//            for (Entry<Queue, LocalQueue> entry : qMap.entrySet()) {
//
//                Queue queue = entry.getKey();
//
//                if (!queue.tryLock()) { // if queue is locked by another thread
//                    abort = true;
//                    break;
//
//                }
//
//                LocalQueue lQueue = entry.getValue();
//                lQueue.isLockedByMe = true;
//
//            }
//        }

        // validate read set

        auto& readSet = localStorage.readSet;

        if (!abort) {

            for (auto node : readSet) {
                if (lockedLNodes.count(node) == 0 && node->isLocked()) {
                    // someone else holds the lock
                    abort = true;
                    break;
                } else if (node->getVersion() > local_transaction.readVersion) {
                    abort = true;
                    break;
                } else if (node->getVersion() == local_transaction.readVersion && node->isSingleton()) {
                    incrementAndGetVersion(); // increment GVC
                    node->setSingleton(false);
                    abort = true;
                    break;
                }
            }
        }

        // validate queue TODO implment

//        if (!abort) {
//            for (Entry<Queue, LocalQueue> entry : qMap.entrySet()) {
//
//                Queue queue = entry.getKey();
//                if (queue.getVersion() > localStorage.readVersion) {
//                    abort = true;
//                    break;
//                } else if (queue.getVersion() == localStorage.readVersion && queue.isSingleton()) {
//                    incrementAndGetVersion(); // increment GVC
//                    abort = true;
//                    break;
//                }
//
//            }
//
//        }

        // increment GVC

        uint64_t writeVersion = 0;

        if (!abort && !local_transaction.readOnly) {
            writeVersion = incrementAndGetVersion();
            assert (writeVersion > local_transaction.readVersion);
            local_transaction.writeVersion = writeVersion;
        }

        // commit
        if (!abort && !local_transaction.readOnly) {
            // LinkedList
            for (auto node_and_we : writeSet) {
                node_t node = node_and_we.first;
                auto we =  node_and_we.second;
                node->m_next = we.next;
                node->m_val = we.val; // when node val changed because of put
                if (we.deleted) {
                    node->setDeleted(true);
                    node->m_val = NULLOPT; // for index
                }
                node->setVersion(writeVersion);
                node->setSingleton(false);
            }
        }

        //TODO implment
//        if (!abort) {
//            // Queue
//
//            for (Entry<Queue, LocalQueue> entry : qMap.entrySet()) {
//
//                Queue queue = entry.getKey();
//                LocalQueue lQueue = entry.getValue();
//
//                queue.dequeueNodes(lQueue.nodeToDeq);
//                queue.enqueueNodes(lQueue);
//                if (TX.DEBUG_MODE_QUEUE) {
//                    System.out.println("commit queue before set version");
//                }
//                queue.setVersion(writeVersion);
//                queue.setSingleton(false);
//
//            }
//
//        }

        // release locks, even if abort
        for (auto node : lockedLNodes) {
            node->unlock();
        }

        //TODO implment

//        for (Entry<Queue, LocalQueue> entry : qMap.entrySet()) {
//
//            Queue queue = entry.getKey();
//            LocalQueue lQueue = entry.getValue();
//            if (lQueue.isLockedByMe) {
//                queue.unlock();
//                lQueue.isLockedByMe = false;
//            }
//        }

        // update index
        if (!abort && !local_transaction.readOnly) {
            // adding to index
            auto& indexMap = localStorage.indexAdd;
            for (auto& list_and_node : indexMap) {
                auto& list = list_and_node.first;
                auto& nodes = list_and_node.second;
                for (auto &node : nodes) {
                    list->index.add(node);
                }
            }
            // removing from index
            for (auto& list_and_node : localStorage.indexRemove) {
                auto& list = list_and_node.first;
                auto& nodes = list_and_node.second;
                for (auto &node : nodes) {
                    list->index.remove(node);
                }
            }
        }

        // cleanup

//        localStorage.queueMap.clear();
        localStorage.writeSet.clear();
        localStorage.readSet.clear();
        localStorage.indexAdd.clear();
        localStorage.indexRemove.clear();
        local_transaction.TX = false;
        local_transaction.readOnly = true;

//        if (DEBUG_MODE_TX) {
//            if (abort) {
//                System.out.println("TXend - aborted");
//            }
//            System.out.println("TXend - is done");
//        }

        if (abort) {
            throw TxAbortException();
        }

        return true;

    }
};