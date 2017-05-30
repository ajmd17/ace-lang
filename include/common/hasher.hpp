#ifndef HASHER_HPP
#define HASHER_HPP

#include <cstdint>

inline std::uint32_t hash_fnv_1(const char *str)
{
    const std::uint32_t PRIME = 16777619u;
    const std::uint32_t OFFSET_BASIS = 2166136261u;

    std::uint32_t hash = OFFSET_BASIS;

    char c = 0;
    while ((c = *str++)) {
        hash *= PRIME;
        hash ^= c;
    }

    return hash;
}

#endif