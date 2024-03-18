#pragma once

#include <functional>
#include <tuple>
#include <iostream>

namespace dp {

// based on the boost library (https://stackoverflow.com/a/2595226/11983817)
template<typename T>
void hash_combine(std::size_t &seed, const T &v) {
    std::hash<T> hash;
    seed ^= hash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// based on https://stackoverflow.com/a/9248150/11983817
template<size_t N, typename... T>
void print_tuple(std::ostream &os, const std::tuple<T...> &tup) {
    if constexpr (N < sizeof...(T)) {
        if constexpr (N > 0) {
            os << ", ";
        }
        os << std::get<N>(tup);
        print_tuple<N + 1>(os, tup);
    }
}

template<typename... T>
std::ostream &operator<<(std::ostream &os, const std::tuple<T...> &tup) {
    os << "[";
    print_tuple<0>(os, tup);
    return os << "]";
}

} // namespace
