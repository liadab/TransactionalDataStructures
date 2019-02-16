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
    node_t n(5, 3);
    Index<size_t, size_t > my_index(n);
    Rand *r = new Rand(1, 200);
    std::cout << r->get() << std::endl;
    std::shared_ptr<TX> tx = std::make_shared<TX>();
    LinkedList<size_t, size_t> l(tx);

    std::vector<size_t> inputs_keys = {1, 2, 3};
    std::vector<size_t> inputs_vals = {4, 5, 6};

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, &l, &inputs_keys, &inputs_vals]() {
           l.put(inputs_keys[i], inputs_vals[i]);
        });
    }
    for (auto &t : threads) {
        t.join();
    }
    l.put(5, 3);

    std::cout << l << std::endl;
    return 0;
}