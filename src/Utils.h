#pragma once
#include <memory>
#include <compare>
#include <utility>
#include <concepts>
#include <range/v3/all.hpp>

#define utf8(str) reinterpret_cast<const char*>(u8##str)

using namespace std::literals;
namespace views = ranges::views;


namespace util {

template <typename T = size_t>
constexpr auto enumerate = ::views::enumerate | ::views::transform([](auto&& pair) {
    using second_type = decltype(pair.second);
    return std::pair<T, second_type>{ static_cast<T>(pair.first), std::forward<second_type>(pair.second) };
});

template <typename T>
constexpr auto iota(std::integral auto&& begin, std::integral auto&& end) {
    return views::iota(static_cast<T>(begin), static_cast<T>(end));
}

template <typename T>
auto is_same(T&& value) {
    using capture_type = std::add_const_t<std::remove_reference_t<T>>;
    return [value = capture_type{ value }](const auto& other) {
        return value == other;
    };
}

template <typename T>
auto is_same_ref(T&& value) {
    using capture_type = std::add_lvalue_reference_t<std::add_const_t<std::remove_reference_t<T>>>;
    return [value = capture_type{ value }](const auto& other) {
        return value == other;
    };
}
} // namespace util
