//
// Created by crocdialer on 05.02.23.
//

#pragma once

#include <functional>

namespace kinski
{

template<class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
}

template<class T, class U>
struct pair_hash
{
    inline size_t operator()(const std::pair<T, U> &p) const
    {
        size_t h = 0;
        kinski::hash_combine(h, p.first);
        kinski::hash_combine(h, p.second);
        return h;
    }
};

}// namespace kinski