#include <gtest/gtest.h>
#include "../datatypes/LinkedList.h"

TEST(LinkedListSingelton, putOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(1);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    auto r1 = l.put(5, 3, record_mgr);
    EXPECT_EQ(l.get(5, record_mgr), 3);
    EXPECT_EQ(r1, NULLOPT);
    auto r2 = l.put(5, 4, record_mgr);
    EXPECT_EQ(l.get(5, record_mgr), 4);
    EXPECT_EQ(r2, 3);
}

TEST(LinkedListSingelton, removeOne) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(1);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    auto r1 = l.remove(5, record_mgr);
    EXPECT_EQ(r1, NULLOPT);
    l.put(5, 3, record_mgr);
    auto r2 = l.remove(5, record_mgr);
    EXPECT_EQ(r2, 3);
    EXPECT_EQ(l.get(5, record_mgr), NULLOPT);
}

TEST(LinkedListSingelton, putMany) {
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(1);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    LinkedList<size_t, size_t> l(tx, record_mgr);
    l.put(5, 3, record_mgr);
    l.put(1, 4, record_mgr);
    l.put(2, 6, record_mgr);
    l.put(8, 10, record_mgr);
    EXPECT_EQ(l.get(5, record_mgr), 3);
    EXPECT_EQ(l.get(1, record_mgr), 4);
    EXPECT_EQ(l.get(2, record_mgr), 6);
    EXPECT_EQ(l.get(8, record_mgr), 10);
}
