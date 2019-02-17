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

    template <typename key_t, typename val_t>
    void TXbegin() {
        if (DEBUG_MODE_TX) {
            std::cout << "begin transction" << std::endl;
        }

        auto& localStorage = get_local_storge<key_t, val_t>();
        localStorage.TX = true;
        localStorage.readVersion = getVersion();
    }

};