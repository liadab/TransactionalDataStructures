#include <iostream>

#include "nodes/LNode.h"
#include "nodes/LNodeWrapper.h"
#include "datatypes/LinkedList.h"
//#include "nodes/utils.h"
//#include "nodes/Index.h"


int main() {
    using node_t = LNodeWrapper<size_t,size_t>;

    std::cout << "Hello, World!" << std::endl;
    node_t n(5, 3);
//    Index<int, int> my_index = new Index<int, int>(n);
//    Rand *r = new Rand(1, 200);
//    std::cout << r->get() << std::endl;
    std::shared_ptr<TX> tx;
    LinkedList<size_t, size_t> l(tx);
    l.put(5, 3);
    std::cout << l << std::endl;

    return 0;
}