#pragma once
#include <unordered_map>
#include <range/v3/all.hpp>
#include <nlohmann/json.hpp>
#include "Application.h"

#define utf8(str) reinterpret_cast<const char*>(u8##str)


using namespace std::literals;
namespace views = ranges::views;
using json = nlohmann::json;

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


struct Food {
    std::string name;
    std::array<float, 5> values = { 0.0f };

    static constexpr auto value_names = std::array{
        "protein"sv, "carbo"sv, "fat"sv, "calories"sv
    };

    enum ValueIndex : size_t {
        Protein = 0,
        Carbo,
        Fat,
        Calories,
        Weight
    };
};

struct FoodProps {
    std::array<float, 5> props = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    [[nodiscard]] constexpr float get_value_from_weight(const Food::ValueIndex value, const float weight) const noexcept {
        return (weight * props[value] / props[Food::Weight]);
    }

    [[nodiscard]] constexpr float get_value_from_weight(const size_t value, const float weight) const noexcept {
        return get_value_from_weight(static_cast<Food::ValueIndex>(value), weight);
    }

    [[nodiscard]] constexpr float get_weight_from_value(const Food::ValueIndex value, const float weight) const noexcept {
        return (weight * props[Food::Weight] / props[value]);
    }

    [[nodiscard]] constexpr float get_weight_from_value(const size_t value, const float weight) const noexcept {
        return get_weight_from_value(static_cast<Food::ValueIndex>(value), weight);
    }
};

class NutritionTracker : public Application {
public:
    NutritionTracker();

private:
    void on_update(double dt) override;

private:
    std::unordered_map<std::string, FoodProps> m_food_props;
};
