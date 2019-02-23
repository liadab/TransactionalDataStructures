#pragma once

#include <cstdint>
#include <thread>

#define CAT2(x, y) x##y
#define CAT(x, y) CAT2(x, y)
#define PAD volatile char CAT(___padding, __COUNTER__)[128]

class RandomFNV1A {
private:
    union {
        PAD;
        uint64_t seed;
    };
public:
    RandomFNV1A() {
        this->seed = 0;
    }
    RandomFNV1A(uint64_t seed) {
        this->seed = seed;
    }

    size_t next() {
        uint64_t offset = 14695981039346656037ULL;
        uint64_t prime = 1099511628211;
        uint64_t hash = offset;
        hash ^= seed;
        hash *= prime;
        seed = hash;
        return hash;
    }
};

size_t get_random_in_range(size_t min_range, size_t max_range);
