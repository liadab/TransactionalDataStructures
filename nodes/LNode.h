#pragma once

#include <cstdint>
#include <atomic>
#include <memory>

template <typename key_t, typename val_t>
class LNode {
public:

private:
    bool tryLock() {
        uint64_t l = m_version_mask;
        if ((l & LOCK_MASK) != 0) {
            return false;
        }
        uint64_t locked = l | LOCK_MASK;
        return m_version_mask.compareAndSet(l, locked);
    }

    void unlock() {
        uint64_t l = m_version_mask.get();
        assert ((l & LOCK_MASK) != 0 && "unlocking a node that is not locked");
        uint64_t  unlocked = l & (~LOCK_MASK);
        bool ret = m_version_mask.compareAndSet(l, unlocked);
        assert (ret);
    }

    bool isLocked() {
        uint64_t l = m_version_mask.get();
        return (l & lockMask) != 0;
    }

    bool isDeleted() {
        uint64_t l = m_version_mask.get();
        return (l & deleteMask) != 0;
    }

    void setDeleted(bool value) {
        uint64_t l = m_version_mask.get();
        assert ((l & lockMask) != 0);
        if (value) {
            l |= deleteMask;
            m_version_mask.set(l);
            return;
        }
        l &= (~deleteMask);
        m_version_mask.set(l);
    }

    bool isLockedOrDeleted() {
        uint64_t l = m_version_mask.get();
        return ((l & deleteMask) != 0) || ((l & lockMask) != 0);
    }
    
    static constexpr uint64_t LOCK_MASK = 0x1000000000000000L;
    static constexpr uint64_t DELETE_MASK = 0x1000000000000000L;
    static constexpr uint64_t SINGLETON_MASK = 0x1000000000000000L;
    static constexpr uint64_t VERSIONNEG_MASK = LOCK_MASK | DELETE_MASK | SINGLETON_MASK;
    std::unique_ptr<key_t> m_key;
    std::unique_ptr<val_t> m_val;
    LNode<key_t, val_t> m_next;
    std::atomic<uint64_t> m_version_mask;
};

#include "LNode.inl"