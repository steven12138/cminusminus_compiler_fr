#pragma once
#include <cstddef>


inline size_t hash_combine(size_t x, size_t y) {
    return y ^ (x + 0x9e3779b97f4a7c15ull + (y << 6) + (y >> 2));
}


template<typename T1, typename T2, typename T1_Hash = std::hash<T1>, typename T2_Hash = std::hash<T2> >
struct PairHash {
    size_t operator()(const std::pair<T1, T2> &p) const noexcept {
        const size_t h1 = T1_Hash{}(p.first);
        const size_t h2 = T2_Hash{}(p.second);
        return hash_combine(h1, h2);
    }
};
