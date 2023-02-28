#pragma once

#include <functional>
#include <tuple>
#include <iostream>
#include <type_traits>

namespace dp {

// based on the boost library (https://stackoverflow.com/a/2595226/11983817)
template <typename T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// based on https://stackoverflow.com/a/9248150/11983817
template <size_t n, typename... T>
typename std::enable_if<(n >= sizeof...(T))>::type
    print_tuple(std::ostream&, const std::tuple<T...>&)
{}

template <size_t n, typename... T>
typename std::enable_if<(n < sizeof...(T))>::type
    print_tuple(std::ostream& os, const std::tuple<T...>& tup)
{
    if (n != 0)
        os << ", ";
    os << std::get<n>(tup);
    print_tuple<n+1>(os, tup);
}

template <typename... T>
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& tup)
{
    os << "[";
    print_tuple<0>(os, tup);
    return os << "]";
}

} // namespace
