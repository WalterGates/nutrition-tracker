#pragma once
#include <unordered_map>
#include <range/v3/all.hpp>
#include <nlohmann/json.hpp>
#include "Utils.h"
#include "Application.h"

using json = nlohmann::json;


struct Food {
    std::string name;
    std::array<float, 5> values = { 0.0f };

    static constexpr auto value_names = std::array{ "protein"sv, "carbo"sv, "fat"sv, "calories"sv };

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

    [[nodiscard]] constexpr float get_value_from_weight(const size_t value_index, const float weight) const noexcept {
        return get_value_from_weight(static_cast<Food::ValueIndex>(value_index), weight);
    }

    [[nodiscard]] constexpr float get_weight_from_value(const Food::ValueIndex value, const float weight) const noexcept {
        return (weight * props[Food::Weight] / props[value]);
    }

    [[nodiscard]] constexpr float get_weight_from_value(const size_t value_index, const float weight) const noexcept {
        return get_weight_from_value(static_cast<Food::ValueIndex>(value_index), weight);
    }
};

class NutritionTracker : public Application {
public:
    NutritionTracker();

private:
    void on_update(double dt) override;

private:
    std::unordered_map<std::string, FoodProps> m_food_props;
    std::vector<Food> m_rows;
};
