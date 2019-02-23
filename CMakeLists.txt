cmake_minimum_required(VERSION 2.8)
project(TransactionalDataStructures)

set(CMAKE_CXX_C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x -g")

add_executable(tds main.cpp nodes/LNode.h datatypes/dummyIndex.h nodes/utils.cpp TX.h optional.h)

#uncomment this to use jmalloc
#target_link_libraries(tds jemalloc)


add_subdirectory(test)