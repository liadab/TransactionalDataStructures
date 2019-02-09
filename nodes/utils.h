#pragma once

#include <random>

class Rand {
public:
    std::uniform_int_distribution<int> m_dist;

    Rand(int min_range, int max_range) :
    m_engine(m_rd), m_dist(min_range, max_range){};
    int get() {
        return m_dist(m_engine);
    }

private:
    std::random_device m_rd;
    std::default_random_engine m_engine;
};
