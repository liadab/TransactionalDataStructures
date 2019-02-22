#include <gtest/gtest.h>
#include <thread>
#include "../datatypes/LinkedList.h"

class ThreadRunner {
public:
    template <typename func1_t, typename func2_t>
    void run_thread_set_1(func1_t func1, func2_t func2) {
        stop_main = true;
        stop_thread = true;
        m_t.emplace([this, func1, func2]() {
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
    std::optional<std::thread> m_t;
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
    EXPECT_EQ(r2, std::nullopt);
    //now there will be a commit
    t1.run_thread_set_2();
    ASSERT_THROW(l.get(5), TxAbortException);
}
