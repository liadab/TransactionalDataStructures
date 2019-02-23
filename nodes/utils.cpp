#include "utils.h"

size_t get_random_in_range(size_t min_range, size_t max_range) {
    static thread_local RandomFNV1A local_rand = RandomFNV1A(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return local_rand.next() % (max_range - min_range) + min_range;
}
