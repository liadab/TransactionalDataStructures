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
    LinkedList<size_t, size_t> l(tx);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx] {
        tx->TXbegin();
        auto r1 = l.put(5, 3);
    },
            [&l, tx] {
        tx->TXend<size_t, size_t>();
    });
    auto r2 = l.get(5);
    EXPECT_EQ(r2, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
    ASSERT_THROW(l.get(5), TxAbortException);
}

TEST(LinkedListTransctionMT, putOneWithSingelton) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx] {
                            tx->TXbegin();
                            auto r1 = l.put(5, 3);
                        },
                        [&l, tx] {
                            tx->TXend<size_t, size_t>();
                        });
    auto r2 = l.get(5);
    EXPECT_EQ(r2, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
    auto r3 = l.get(5);
    EXPECT_EQ(r3, 3);
}

//this is bug there should be an abort
TEST(LinkedListTransctionMT, SingeltonPutTxAbort) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx] {
                            tx->TXbegin();
                            auto r1 = l.get(5);
                            EXPECT_EQ(r1, NULLOPT);
                        },
                        [&l, tx] {
                            ASSERT_THROW(l.get(5), TxAbortException);
                        });
    auto r1 = l.put(5, 3);
    EXPECT_EQ(r1, NULLOPT);
    //now there will be a commit
    t1.run_thread_set_2();
}

//this is bug there should be an abort
TEST(LinkedListTransctionMT, SingeltonPutBeforeTx) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    auto r1 = l.put(5, 3);
    EXPECT_EQ(r1, NULLOPT);
    ThreadRunner t1;
    t1.run_thread_set_1([&l, tx] {
        //since there were a singleton put we must abort once in this implmention
                            tx->TXbegin();
                            ASSERT_THROW(l.get(5), TxAbortException);
                            tx->TXbegin();
                            EXPECT_EQ(l.get(5), 3);
                        },
                        [&l, tx] {
                        });
    //now there will be a commit
    t1.run_thread_set_2();
}