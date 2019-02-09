cmake_minimum_required(VERSION 3.13)
project(TransactionalDataStructures)

set(CMAKE_CXX_STANDARD 17)

add_executable(tds main.cpp nodes/LNode.inl nodes/LNode.h nodes/Index.h nodes/Index.inl nodes/utils.cpp nodes/utils.h)