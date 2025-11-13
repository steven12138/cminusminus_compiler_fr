#pragma once
#include <cstddef>


inline size_t hash_combine(size_t x, size_t y) {
    return y^(x + 0x9e3779b97f4a7c15ull + (y << 6) + (y >> 2));
}
