#include <iostream>
#include <thread>

#include "nodes/LNode.h"
#include "nodes/LNodeWrapper.h"
#include "datatypes/LinkedList.h"
//#include "nodes/utils.h"
//#include "nodes/Index.h"


int main() {
    using node_t = LNodeWrapper<size_t,size_t>;
    size_t num_threads = 2;

    std::cout << "Hello, World!" << std::endl;
    node_t n(0, 0);
    Index<size_t, size_t > my_index(n);
//    node_t n2(3, 9);
//    node_t n3(7, 0);
//    node_t n4(8, 8);
//    my_index.add(n2);
//    my_index.add(n3);
//    my_index.add(n4);
    node_t n5(5, 18);
    my_index.add(n5);
    for (size_t i = 0; i< 16; i++) {
        if (i==5) continue;
        node_t n(i,  i+1);
        my_index.add(n);
    }

    std::cout << "before remove" << std::endl;
    std::cout << my_index << std::endl;
    try {
        n5->m_val = NULL;
        my_index.remove(n5);
    } catch (std::exception& e) {
        std::cout << "remove threw an exception: " << e.what() << std::endl;
    }
    std::cout << "after remove" << std::endl;
    std::cout << my_index << std::endl;

    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);

    std::vector<size_t> inputs_keys = {1, 2, 3};
    std::vector<size_t> inputs_vals = {4, 5, 6};
//    std::vector<std::thread> threads;
//    for (size_t i = 0; i < num_threads; ++i) {
//        threads.emplace_back([i, &l, &inputs_keys, &inputs_vals]() {
//           l.put(inputs_keys[i], inputs_vals[i]);
//        });
//    }
//    for (auto &t : threads) {
//        t.join();
//    }

    l.put(5, 3);

    std::cout << l << std::endl;
    return 0;
}