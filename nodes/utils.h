#pragma once

#include <random>

class Rand {
public:

    Rand(size_t min_range, size_t max_range);
    size_t get();

private:
    size_t m_min_range;
    size_t m_max_range;
};
