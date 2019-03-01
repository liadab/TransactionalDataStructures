#include <gtest/gtest.h>
#include "../nodes/Index.h"
#include "../nodes/record_mgr.h"

TEST(IndexBasic, putAndRemoveMulti) {
    using node_t = LNodeWrapper<size_t,size_t>;
    auto global_record_mgr = RecordMgr<size_t, size_t>::make_record_mgr(1);
    RecordMgr<size_t, size_t> record_mgr(global_record_mgr, 0);
    auto n = record_mgr.get_new_node(std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::min());
    Index<size_t, size_t> ind(n);
    std::vector<node_t> nodes(32);
    for (size_t i = 0; i < 32; i++) {
        auto n = record_mgr.get_new_node(i+1,  i+1);
        nodes[i] = n;
        ind.add(n);
    }
    std::cout << "before remove" << std::endl;
    std::cout << ind << std::endl;

    for (size_t i = 0; i < 32; i++) {
        auto n = nodes[i];
        n->m_val = NULLOPT;
        ind.remove(n);
    }
    std::cout << "after remove" << std::endl;
    std::cout << ind << std::endl;
}

//TEST(IndexBasic, insertionPoint) {
//    using node_t = LNodeWrapper<size_t,size_t>;
//    node_t n(std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::min());
//    Index<size_t, size_t> ind(n);
//    std::vector<node_t> nodes(32);
//    for (size_t i = 0; i < 32; i++) {
//        if (i != 12) {
//            node_t n(i + 1, i + 1);
//            nodes[i] = n;
//            ind.add(n);
//        }
//    }
//    std::cout << "after insertion" << std::endl;
//    std::cout << ind << std::endl;
//
//    node_t n(13, 13);
//    std::vector<std::shared_ptr<Index::IndexNode>> prevs;
//    std::vector<std::shared_ptr<Index::IndexNode>> nexts;
//    std::tie(prevs, nexts) = ind.findInsertionPoints(n);
//    for (int i = 0; i < prevs.size(); ++i) {
//        std::cout << "level: " << i << "prev: " << prevs[i]->m_node << "next: " << nexts[i]->m_node << std::endl;
//    }
//}
