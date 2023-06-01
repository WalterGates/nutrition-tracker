#include "NutritionTracker.h"

#include <fstream>
#include <imgui.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <range/v3/view/enumerate.hpp>
#include "imgui_combo_autoselect.h"


std::unique_ptr<Application> create_application() {
    return std::make_unique<NutritionTracker>();
}

NutritionTracker::NutritionTracker() {
    auto food_json = json::parse(std::ifstream{ "res/database.json" });

    for (const auto& [food_name, json_props] : food_json.items()) {
        auto is_valid   = (json_props.size() == Food::value_names.size());
        auto food_props = FoodProps{};

        // FIXME: using the enumerated index as the positional key, this will break if the props are out of order in the json
        // file
        for (auto&& [index, name] : Food::value_names | views::enumerate) {
            is_valid &= (json_props.contains(name) && json_props[name].is_number());
            if (!is_valid) {
                break;
            }
            food_props.props[index] = json_props[name].get<float>();
        }

        if (!is_valid) {
            fmt::print(stderr, fmt::fg(fmt::color::red), "[ERROR]: Invalid json format inside the node '{}'\n", food_name);
            return;
        }

        m_food_props.emplace(food_name, food_props);
    }
}

void NutritionTracker::on_update(double /*dt*/) {
    // TODO: Make windows pop out
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Begin("Meal Window");

    static auto rows = std::vector<Food>{};
    static auto data = [&] {
        const auto food_names =
            m_food_props | views::transform([](auto&& pair) { return std::forward<decltype(pair.first)>(pair.first); });
        return ImGui::ComboAutoSelectData{ { food_names.begin(), food_names.end() } };
    }();

    // static auto data = ImGui::ComboAutoSelectData{ {
    //     utf8("ton"),
    //     utf8("orez"),
    //     utf8("pâine"),
    //     utf8("salată"),
    //     utf8("paste"),
    //     utf8("fasole"),
    //     utf8("ouă"),
    //     utf8("lapte"),
    //     utf8("morcovi"),
    //     utf8("ceapă"),
    //     utf8("ardei"),
    //     utf8("cașcaval"),
    // } };

    { // Table for inputing meals
        const auto table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;

        // TODO: Make table sortable
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.0f, 0.0f });
        if (ImGui::BeginTable("table1", 6, table_flags)) {

            ImGui::TableSetupColumn("Food");
            ImGui::TableSetupColumn("Protein");
            ImGui::TableSetupColumn("Carbo");
            ImGui::TableSetupColumn("Fat");
            ImGui::TableSetupColumn("Calories");
            ImGui::TableSetupColumn("Weight");
            ImGui::TableHeadersRow();

            for (auto&& [row_index, row] : rows | util::enumerate<int>) {
                ImGui::PushID(row_index);

                auto add_cell = [&, column_index = 0](auto&& widget_fn) mutable {
                    ImGui::TableNextColumn();
                    ImGui::PushID(column_index++);
                    ImGui::SetNextItemWidth(-FLT_MIN);

                    widget_fn();
                    ImGui::PopID();
                };

                add_cell([&] {
                    if (ImGui::ComboAutoSelect("##food_dropdown", data) && data.index != -1) {
                        ranges::fill(row.values, 0.0f);
                    }
                });

                for (auto&& [value_index, value] : row.values | views::enumerate) {
                    add_cell([&] {
                        if (ImGui::DragFloat("##grams_input", &value, 1.0f, 0.0f, 10'000.0f, "%.1fg")) {
                            const auto& food_props = m_food_props[row.name];

                            const auto weight            = food_props.get_weight_from_value(value_index, value);
                            auto other_values_enumerated = row.values | views::enumerate |
                                views::filter([&](const auto& pair) { return &value != &pair.second; });

                            for (auto&& [other_value_index, other_value] : other_values_enumerated) {
                                other_value = food_props.get_value_from_weight(other_value_index, weight);
                            }
                        }
                    });
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    } // Table for inputing meals


    { // Column of delete buttons
        ImGui::SameLine();
        ImGui::BeginGroup();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ FLT_MAX, 0.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.0f, 0.0f });

        // Use up the height of the table header
        ImGui::Dummy(ImVec2{ 0.0f, ImGui::GetTextLineHeightWithSpacing() });

        for (const auto& row_index : util::iota<int>(0, rows.size())) {
            ImGui::PushID(row_index);
            ImGui::TableNextColumn();

            if (ImGui::Button("x")) {
                rows.erase(rows.begin() + row_index);
            }
            ImGui::PopID();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndGroup();
    } // Column of delete buttons

    if (ImGui::ComboAutoSelect("Add food", data) && data.index != -1) {
        rows.push_back({
            .name = data.input,
        });
    }

    ImGui::End();
}
