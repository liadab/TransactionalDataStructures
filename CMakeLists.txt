cmake_minimum_required(VERSION 3.13)
project(TransactionalDataStructures)

set(CMAKE_CXX_STANDARD 17)

add_executable(tds main.cpp nodes/LNode.inl nodes/LNode.h nodes/Index.h nodes/Index.inl nodes/utils.h WriteElement.cpp WriteElement.h LocalStorage.cpp LocalStorage.h datatypes/LinkedList.cpp datatypes/LinkedList.h exceptions/abort.h)