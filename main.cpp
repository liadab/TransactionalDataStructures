#include <iostream>

#include "nodes/LNode.h"
#include "nodes/utils.h"
//#include "nodes/Index.h"


int main() {
    std::cout << "Hello, World!" << std::endl;
    size_t a;
    a = 5;
    LNode <int, int> n;
//    Index<int, int> my_index = new Index<int, int>(n);
    std::cout << a << std::endl;
    Rand *r = new Rand(1, 200);
    std::cout << r->get() << std::endl;
    return 0;
}