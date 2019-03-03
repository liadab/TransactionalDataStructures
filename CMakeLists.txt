cmake_minimum_required(VERSION 2.8)
project(TransactionalDataStructures)

set(CMAKE_CXX_C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=gnu++0x -g")

include_directories(common)

add_executable(tds main.cpp nodes/LNode.h datatypes/dummyIndex.h nodes/utils.cpp TX.h optional.h nodes/QNode.h datatypes/Queue.h datatypes/LocalQueue.h nodes/record_mgr.h nodes/log_file_helper.h)
#target_compile_definitions(tds -DSHARED_PTR)
#add_compile_definitions(DEBRA)

add_executable(tds_unsafe main.cpp nodes/LNode.h datatypes/dummyIndex.h nodes/utils.cpp TX.h optional.h nodes/QNode.h datatypes/Queue.h datatypes/LocalQueue.h nodes/record_mgr.h)
set_property(TARGET tds_unsafe PROPERTY COMPILE_DEFINITIONS UNSAFE)

add_executable(tds_debra main.cpp nodes/LNode.h datatypes/dummyIndex.h nodes/utils.cpp TX.h optional.h nodes/QNode.h datatypes/Queue.h datatypes/LocalQueue.h nodes/record_mgr.h)
set_property(TARGET tds_debra PROPERTY COMPILE_DEFINITIONS DEBRA)

#uncomment this to use jmalloc
#target_link_libraries(tds jemalloc)


add_subdirectory(test)