#pragma once

#include <atomic>

class TxAbortException {

};
class TX {
public:
    std::atomic<uint64_t> gvc;

    static constexpr bool DEBUG_MODE_LL = false;
    static constexpr bool DEBUG_MODE_QUEUE = false;
    static constexpr bool DEBUG_MODE_TX = false;
    static constexpr bool DEBUG_MODE_VERSION = false;

    uint64_t getVersion() const {
        return gvc;
    }

    uint64_t incrementAndGetVersion() {
        return gvc++;
    }

    void TXbegin() { }

    template <typename key_t, typename val_t>
    LocalStorage<key_t, val_t>& get_local_storge() const {
        static thread_local  LocalStorage<key_t, val_t> lStorage;
        return lStorage;
    }

};