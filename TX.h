#pragma once

#include <atomic>
#include "LocalTransaction.h"
class TxAbortException {

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
        return gvc++;
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

};