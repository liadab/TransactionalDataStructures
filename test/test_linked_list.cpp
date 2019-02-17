#include <gtest/gtest.h>
#include "../datatypes/LinkedList.h"

TEST(LinkedListTransction, putOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    tx->TXbegin<size_t, size_t>();
    LinkedList<size_t, size_t> l(tx);
    auto r1 = l.put(5, 3);
    EXPECT_EQ(l.get(5), 3);
    EXPECT_EQ(r1, std::nullopt);
    auto r2 = l.put(5, 4);
    EXPECT_EQ(l.get(5), 4);
    EXPECT_EQ(r2, 3);
}

TEST(LinkedListTransction, removeOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    tx->TXbegin<size_t, size_t>();
    LinkedList<size_t, size_t> l(tx);
    auto r1 = l.remove(5);
    EXPECT_EQ(r1, std::nullopt);
    l.put(5, 3);
    auto r2 = l.remove(5);
    EXPECT_EQ(r2, 3);
    EXPECT_EQ(l.get(5), std::nullopt);
}

TEST(LinkedListTransction, putMany) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    tx->TXbegin<size_t, size_t>();
    LinkedList<size_t, size_t> l(tx);
    l.put(5, 3);
    l.put(1, 4);
    l.put(2, 6);
    l.put(8, 10);
    EXPECT_EQ(l.get(5), 3);
    EXPECT_EQ(l.get(1), 4);
    EXPECT_EQ(l.get(2), 6);
    EXPECT_EQ(l.get(8), 10);
}
