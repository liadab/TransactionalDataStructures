#pragma once

#include <mutex>
#include <atomic>
#include "../nodes/QNode.h"

template <typename val_t>
class Queue {
public:

private:
    std::mutex m_lock;
    QNode<val_t> head;
    QNode<val_t> tail;
    int size;
    static constexpr uint64_t LOCK_MASK = 0x1000000000000000L;
    static constexpr uint64_t SINGLETON_MASK = 0x4000000000000000L;
    static constexpr uint64_t VERSIONNEG_MASK = LOCK_MASK | SINGLETON_MASK;
    std::atomic<uint64_t> m_version_mask;

};