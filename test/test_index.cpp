#include <gtest/gtest.h>
#include "../nodes/Index.h"

TEST(IndexBasic, putAndRemoveMulti) {
    using node_t = LNodeWrapper<size_t,size_t>;
    node_t n(std::numeric_limits<size_t>::min(), std::numeric_limits<size_t>::min());
    Index<size_t, size_t> ind(n);
    std::vector<node_t> nodes(32);
    for (size_t i = 0; i < 32; i++) {
        node_t n(i+1,  i+1);
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
