#pragma once
#include <optional>
#include <unordered_map>
#include <range/v3/all.hpp>
#include <nlohmann/json.hpp>
#include "Utils.h"
#include "Application.h"
#include "imgui_combo_autoselect.h"

using json = nlohmann::json;


struct Food {
    std::string name;
    std::array<float, 5> values = { 0.0f };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Food, name, values)

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

using food_values_table_type = std::unordered_map<std::string, FoodProps>;

class EditMealWidget {
public:
    EditMealWidget() = default;
    EditMealWidget(const std::shared_ptr<food_values_table_type>& food_values_table);
    EditMealWidget(const json& json_serial, const std::shared_ptr<food_values_table_type>& food_values_table);

    void draw();
    [[nodiscard]] json serialize();
    EditMealWidget& deserialize(const json& json_serial);

private:
    void draw_text_input();
    void draw_table();
    void draw_remove_buttons();
    void draw_add_food_dropdown();

    bool draw_value_row(Food& row);
    void draw_total_row();

    void reset_ids();
    void next_column(auto&& func);
    void recalculate_total();

private:
    int m_next_id = 0;
    std::vector<Food> m_rows;
    Food m_total_row{ .name = "Total" };

    std::string m_title;
    std::string m_notes;

    ImGui::ComboAutoSelectData m_dropdown_data{ std::vector<std::string>{} };
    std::shared_ptr<food_values_table_type> m_food_values_table;
};

class NutritionTracker : public Application {
public:
    NutritionTracker();

private:
    void on_update(double dt) override;

private:
    EditMealWidget m_edit_meal_widget;
    std::shared_ptr<food_values_table_type> m_food_values_table = std::make_shared<food_values_table_type>();
};
