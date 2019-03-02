#pragma once

//debra
#include <recordmgr/record_manager.h>
#include <recordmgr/allocator_new.h>

#include "LNodeWrapper.h"

#ifdef DEBRA
template <typename key_t, typename val_t>
class RecordMgr {
public:
    using node_t = LNode<key_t, val_t>;
    using record_manager_t = record_manager<reclaimer_debra<key_t>, allocator_new<key_t>, pool_none<key_t>, node_t>;

    static std::shared_ptr<record_manager_t> make_record_mgr(size_t max_threads) {
        return std::make_shared<record_manager_t>(max_threads);
    }
    RecordMgr(std::shared_ptr<record_manager_t> myRecManager, int tid) : myRecManager(myRecManager), tid(tid) {
        myRecManager->initThread(tid);
    }

    ~RecordMgr() {
        myRecManager->deinitThread(tid);
    }

    RecordMgr(const RecordMgr&) = delete;

    auto getGuard() const -> typename record_manager_t::MemoryReclamationGuard {
        return myRecManager->getGuard(tid);
    }

    LNodeWrapper<key_t, val_t> get_new_node(key_t key) const {
        auto myNode = myRecManager->template allocate<node_t>(tid);
        myNode->m_key = key;
        return LNodeWrapper<key_t, val_t>(myNode);
    }

    LNodeWrapper<key_t, val_t> get_new_node(key_t key, val_t val) const {
        auto n = get_new_node(key);
        n->m_val = val;
        return n;
    }

private:
    std::shared_ptr<record_manager_t> myRecManager;
    int tid;
};

#else

//a dummy class in case of no debra
template <typename key_t, typename val_t>
class RecordMgr {
public:
    using node_t = LNode<key_t, val_t>;
    using record_manager_t = record_manager<reclaimer_debra<key_t>, allocator_new<key_t>, pool_none<key_t>, node_t>;

    static std::shared_ptr<record_manager_t> make_record_mgr(size_t max_threads) {
        return std::shared_ptr<record_manager_t>{};
    }

    RecordMgr(std::shared_ptr<record_manager_t> myRecManager, int tid) {
    }

    ~RecordMgr() {
    }

    RecordMgr(const RecordMgr&) = delete;

    size_t getGuard() const {
        return 0;
    }

    LNodeWrapper<key_t, val_t> get_new_node(key_t key) const {
        return LNodeWrapper<key_t, val_t>(std::move(key));
    }

    LNodeWrapper<key_t, val_t> get_new_node(key_t key, val_t val) const {
        auto n = get_new_node(key);
        n->m_val = val;
        return n;
    }

private:
    std::shared_ptr<record_manager_t> myRecManager;
    int tid;
};

#endif
