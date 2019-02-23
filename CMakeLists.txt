cmake_minimum_required(VERSION 2.8)
project(TransactionalDataStructures)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")

add_executable(tds main.cpp nodes/LNode.h datatypes/dummyIndex.h nodes/utils.cpp TX.h)

add_subdirectory(test)