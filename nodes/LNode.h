#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include <cassert>
#include "../optional.h"

template <typename key_t, typename val_t>
class LNodeWrapper;

/**
 * the basic node of the sorted linked list
 * @tparam node_t the way we hold pointers for this list
 */
template <typename key_t, typename val_t>
class LNode {
public:
    using node_t = LNodeWrapper<key_t,val_t>;

    key_t m_key;
    Optional<val_t> m_val;
    node_t m_next;

    //for debra we need the node to be defult ctr
    LNode() : m_key(key_t{}), m_version_mask(0) { }

    explicit LNode(key_t key) : m_key(std::move(key)), m_version_mask(0) {}

    bool tryLock() {
        uint64_t l = m_version_mask;
        if ((l & LOCK_MASK) != 0) {
            return false;
        }
        uint64_t locked = l | LOCK_MASK;
        return m_version_mask.compare_exchange_strong(l, locked);
    }

    void unlock() {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "unlocking a node that is not locked");
        uint64_t  unlocked = l & (~LOCK_MASK);
        bool ret = m_version_mask.compare_exchange_strong(l, unlocked);
        assert (ret && "compare_exchange_strong in unlock failed");
    }

    bool isLocked() {
        uint64_t l = m_version_mask;
        return (l & LOCK_MASK) != 0;
    }

    bool isDeleted() {
        uint64_t l = m_version_mask;
        return (l & DELETE_MASK) != 0;
    }

    void setDeleted(bool value) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "setDeleted on unlocked node");
        if (value) {
            l |= DELETE_MASK;
            m_version_mask = l;
            return;
        }
        l &= (~DELETE_MASK);
        m_version_mask = l;
    }

    bool isLockedOrDeleted() {
        uint64_t l = m_version_mask;
        return ((l & DELETE_MASK) != 0) || ((l & LOCK_MASK) != 0);
    }

    bool isSingleton() {
        uint64_t l = m_version_mask;
        return (l & SINGLETON_MASK) != 0;
    }

    void setSingleton(bool value) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "setSingleton on unlocked node");
        if (value) {
            l |= SINGLETON_MASK;
            m_version_mask = l;
            return;
        }
        l &= (~SINGLETON_MASK);
        m_version_mask = l;
    }

    void setSingletonNoLockAssert(bool value) {
        uint64_t l = m_version_mask;
        if (value) {
            l |= SINGLETON_MASK;
            m_version_mask = l;
            return;
        }
        l &= (~SINGLETON_MASK);
        m_version_mask = l;
    }

    uint64_t getVersion() {
        return (m_version_mask & (~VERSIONNEG_MASK));
    }

    void setVersion(uint64_t version) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0);
        l &= VERSIONNEG_MASK;
        l |= (version & (~VERSIONNEG_MASK));
        m_version_mask = l;
    }

    bool isSameVersionAndSingleton(uint64_t version) {
        uint64_t l = m_version_mask;
        if ((l & SINGLETON_MASK) != 0) {
            l &= (~VERSIONNEG_MASK);
            return l == version;
        }
        return false;
    }

    void setVersionAndSingleton(uint64_t version, bool value) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0);
        l &= VERSIONNEG_MASK;
        l |= (version & (~VERSIONNEG_MASK));
        if (value) {
            l |= SINGLETON_MASK;
            m_version_mask = l;
            return;
        }
        l &= (~SINGLETON_MASK);
        m_version_mask = l;
    }

    void setVersionAndSingletonNoLockAssert(uint64_t version, bool value) {
        uint64_t l = m_version_mask;
        l &= VERSIONNEG_MASK;
        l |= (version & (~VERSIONNEG_MASK));
        if (value) {
            l |= SINGLETON_MASK;
            m_version_mask = l;
            return;
        }
        l &= (~SINGLETON_MASK);
        m_version_mask = l;
    }

    void setVersionAndDeletedAndSingleton(uint64_t version, bool deleted, bool singleton) {
        uint64_t l = m_version_mask;
        assert ((l & LOCK_MASK) != 0 && "setVersionAndDeletedAndSingleton on unlocked node");
        l &= VERSIONNEG_MASK;
        l |= (version & (~VERSIONNEG_MASK));
        if (singleton) {
            l |= SINGLETON_MASK;
        } else {
            l &= (~SINGLETON_MASK);
        }
        if (deleted) {
            l |= DELETE_MASK;
        } else {
            l &= (~DELETE_MASK);
        }
        m_version_mask = l;
    }
private:

    static constexpr uint64_t LOCK_MASK = 0x1000000000000000L;
    static constexpr uint64_t DELETE_MASK = 0x2000000000000000L;
    static constexpr uint64_t SINGLETON_MASK = 0x4000000000000000L;
    static constexpr uint64_t VERSIONNEG_MASK = LOCK_MASK | DELETE_MASK | SINGLETON_MASK;
    std::atomic<uint64_t> m_version_mask;
};

