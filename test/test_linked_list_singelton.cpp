#include <gtest/gtest.h>
#include "../datatypes/LinkedList.h"

TEST(LinkedListSingelton, putOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    auto r1 = l.put(5, 3);
    EXPECT_EQ(l.get(5), 3);
    EXPECT_EQ(r1, NULLOPT);
    auto r2 = l.put(5, 4);
    EXPECT_EQ(l.get(5), 4);
    EXPECT_EQ(r2, 3);
}

TEST(LinkedListSingelton, removeOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    auto r1 = l.remove(5);
    std::cout << "1" << std::endl;

    EXPECT_EQ(r1, NULLOPT);
    std::cout << "2" << std::endl;

    l.put(5, 3);
    std::cout << "3" << std::endl;

    auto r2 = l.remove(5);
    std::cout << "4" << std::endl;

    EXPECT_EQ(r2, 3);
    std::cout << "5" << std::endl;

    EXPECT_EQ(l.get(5), NULLOPT);
    std::cout << "done" << std::endl;
}

TEST(LinkedListSingelton, putMany) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
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
