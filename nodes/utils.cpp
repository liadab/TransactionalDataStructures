#include "utils.h"

size_t Rand::get() {
    return rand() % (m_max_range - m_min_range) + m_min_range;
}

Rand::Rand(size_t min_range, size_t max_range) : m_min_range(min_range), m_max_range(max_range) { }
