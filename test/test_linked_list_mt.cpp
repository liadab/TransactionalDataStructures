#include <gtest/gtest.h>
#include <thread>
#include "../datatypes/LinkedList.h"

class ThreadRunner {
public:
    template <typename func1_t, typename func2_t>
    void run_thread_set_1(func1_t func1, func2_t func2) {
        stop_main = true;
        stop_thread = true;
        m_t = std::make_unique<std::thread>([this, func1, func2]() {
            func1();
            stop_main = false;
            while(stop_thread);
            func2();
        });
        while(stop_main);
    }
    void run_thread_set_2() {
        stop_thread = false;
        m_t->join();
    }

private:
    std::atomic<bool> stop_main;
    std::atomic<bool> stop_thread;
    std::unique_ptr<std::thread> m_t;
};


TEST(LinkedListTransctionMT, putOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    tx->TXbegin();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(2);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    RecordMgr<size_t, size_t> record_mgr2(global_record_mgr, 1);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx, &record_mgr2] {
        tx->TXbegin();
        auto r1 = l.put(5, 3, record_mgr2);
    },
            [&l, tx, &record_mgr2] {
        tx->TXend<size_t, size_t>(record_mgr2);
    });
    auto r2 = l.get(5, record_mgr);
    EXPECT_EQ(r2, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
    ASSERT_THROW(l.get(5, record_mgr), TxAbortException);
}

TEST(LinkedListTransctionMT, putOneWithSingelton) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(2);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    RecordMgr<size_t, size_t> record_mgr2(global_record_mgr, 1);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx, &record_mgr2] {
                            tx->TXbegin();
                            auto r1 = l.put(5, 3, record_mgr2);
                        },
                        [&l, tx, &record_mgr2] {
                            tx->TXend<size_t, size_t>(record_mgr2);
                        });
    auto r2 = l.get(5, record_mgr);
    EXPECT_EQ(r2, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
    auto r3 = l.get(5, record_mgr);
    EXPECT_EQ(r3, 3);
}

//this is bug there should be an abort
TEST(LinkedListTransctionMT, SingeltonPutTxAbort) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(2);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    RecordMgr<size_t, size_t> record_mgr2(global_record_mgr, 1);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx, &record_mgr2] {
                            tx->TXbegin();
                            auto r1 = l.get(5, record_mgr2);
                            EXPECT_EQ(r1, NULLOPT);
                        },
                        [&l, tx, &record_mgr2] {
                            ASSERT_THROW(l.get(5, record_mgr2), TxAbortException);
                        });
    auto r1 = l.put(5, 3, record_mgr);
    EXPECT_EQ(r1, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
}

//this is bug there should be an abort
TEST(LinkedListTransctionMT, SingeltonPutBeforeTx) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(2);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    RecordMgr<size_t, size_t> record_mgr2(global_record_mgr, 1);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    auto r1 = l.put(5, 3, record_mgr);
    EXPECT_EQ(r1, NULLOPT);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx, &record_mgr2] {
        //since there were a singleton put we must abort once in this implmention
                            tx->TXbegin();
                            ASSERT_THROW(l.get(5, record_mgr2), TxAbortException);
                            tx->TXbegin();
                            EXPECT_EQ(l.get(5, record_mgr2), 3);
                        },
                        [&l, tx] {
                        });
    //now there will be a commit
    t1.run_thread_set_2();
}