#include <gtest/gtest.h>
#include "../datatypes/LinkedList.h"

template <typename T>
std::ostream& operator<< (std::ostream& stream, const std::optional<T>& v) {
    if(!v) {
        stream << "None";
    } else {
        stream << *v;
    }
    return stream;
}

TEST(LinkedListSingelton, putOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    l.put(5, 3);
    EXPECT_EQ(l.get(5), 3);
}

TEST(LinkedListSingelton, removeOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);
    l.put(5, 3);
    l.remove(5);
    EXPECT_EQ(l.get(5), std::nullopt);
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
