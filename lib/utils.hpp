#pragma once

#include <type_traits>
#include <functional>
#include <tuple>
#include <iostream>

namespace dp {

// utilities for enum compile-time checks
/**
 * Checks if Value is a valid value.
 * Useful for checking if an unknown enumeration type has a certain value.
 */
template<auto Value, typename = std::void_t<>>
struct is_valid_value : std::false_type {};

template<auto Value>
struct is_valid_value<Value, std::void_t<decltype(Value)>> : std::true_type {};

template<auto Value>
constexpr bool is_valid_value_v = is_valid_value<Value>::value;

/**
 * Specifies that a type is an enumeration type.
 */
template<typename Enum>
concept IsEnum = std::is_enum_v<Enum>;

/**
 * Converts an enumeration value to its underlying type.
 */
template<IsEnum Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

/**
 * Combines a given value's has with a given hash.
 *
 * Based on the boost library (https://stackoverflow.com/a/2595226/11983817)
 *
 * @param seed Preexisting hash.
 * @param v Value who's hash is combined with the preexisting hash.
 */
template<typename T>
void hash_combine(std::size_t &seed, const T &v) {
    std::hash<T> hash;
    seed ^= hash(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


/**
 * Utility for printing tuples into a stream.
 *
 * Based on https://stackoverflow.com/a/9248150/11983817
 */
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
