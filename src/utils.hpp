#pragma once

#include <functional>

namespace dp {

// based on the boost library (https://stackoverflow.com/a/2595226/11983817)
template <typename T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace
