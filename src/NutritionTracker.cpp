#include "NutritionTracker.h"

#include <fstream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <range/v3/view/enumerate.hpp>
#include "imgui_combo_autoselect.h"
#include <csignal>


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

    { // Table for inputting meals
        auto add_row = [row_index = 0](auto&& row_fn) mutable {
            auto add_cell = [column_index = 0]<typename Fn, typename... Args>(Fn && widget_fn, Args && ... args) mutable
                requires std::invocable<Fn, Args...>
            {
                ImGui::TableNextColumn();
                ImGui::PushID(column_index++);
                ImGui::SetNextItemWidth(-FLT_MIN);

                std::invoke(std::forward<Fn>(widget_fn), std::forward<Args>(args)...);
                ImGui::PopID();
            };

            ImGui::PushID(row_index++);
            row_fn(add_cell);
            ImGui::PopID();
        };

        const auto column_names = std::array{ "Food"sv, "Protein"sv, "Carbo"sv, "Fat"sv, "Calories"sv, "Weight"sv };
        const auto table_flags  = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;

        // TODO: Make table sortable and scrollable
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.0f, 0.0f });
        if (ImGui::BeginTable("table1", column_names.size(), table_flags)) {

            for (const auto& name : column_names) {
                ImGui::TableSetupColumn(name.data());
            }
            ImGui::TableHeadersRow();

            static auto total_row = Food{ .name = "Total" };
            auto table_edited     = false;

            for (auto&& row : m_rows) {
                const auto row_name = [&]() {
                    if (ImGui::ComboAutoSelect("##food_dropdown", data) && data.index != -1) {
                        ranges::fill(row.values, 0.0f);
                    }
                };

                const auto row_value = [&](const std::size_t index, float& value) {
                    if (ImGui::DragFloat("##grams_input", &value, 1.0f, 0.0f, 10'000.0f, "%.1fg")) {
                        const auto& food_props = m_food_props[row.name];
                        const auto weight      = food_props.get_weight_from_value(index, value);

                        // clang-format off
                        auto other_values = row.values
                            | views::enumerate
                            | views::filter([&](const auto& pair) { return &value != &pair.second; });
                        // clang-format on

                        for (auto&& [other_index, other_value] : other_values) {
                            other_value = food_props.get_value_from_weight(other_index, weight);
                        }

                        table_edited = true;
                    }
                };

                add_row([&](auto&& add_cell) {
                    add_cell(row_name);
                    for (auto&& [index, value] : row.values | views::enumerate) {
                        add_cell(row_value, index, value);
                    }
                });
            }

            if (table_edited) {
                ranges::fill(total_row.values, 0.0f);

                for (const auto& row : m_rows) {
                    for (const auto& index : views::iota(0u, row.values.size())) {
                        total_row.values[index] += row.values[index];
                    }
                }
            }

            // TODO: Make this row be fixed when adding scrolling to the table. Maybe even make it a different color
            const auto row_name = [&]() {
                // HACK: `ImGui::InputText` looks the nicest but I don't like the fact that i can select it.
                // That on its own isn't all that bad but I hate seeing the cursor blinking. I think
                // `ImGui::Text` should be the best option if I can set the background to be blue.

                // ImGui::TextUnformatted(total_row.name.c_str());
                // ImGui::Selectable(total_row.name.c_str(), true);
                ImGui::InputText("total_row_name", total_row.name.data(), total_row.name.size(), ImGuiInputTextFlags_ReadOnly);
            };

            const auto row_value = [&](float& value, const float prev_value) {
                // HACK: ImGui::DragFloat is triggered on every input change. This is the desired behavior for drag
                // input but for key input, if you would enter something like `0.5`, this would be triggered first
                // for just the `0` and thus it would multiply the whole table by 0.
                // For now I fixed this by forcing a minimum input value, but this still could end up multiplying
                // by a small value then by something really small then by something bigger, thus loosing precision.

                if (ImGui::DragFloat("##grams_input", &value, 1.0f, 0.1f, 10'000.0f, "%.1fg")) {
                    // Guard for division by zero
                    if (std::abs(prev_value) < std::numeric_limits<float>::epsilon()) {
                        value = 0.0f;
                        return;
                    }

                    // clang-format off
                    auto row_values = total_row.values
                        | views::filter([&](const auto& other_value) { return &value != &other_value; });

                    auto table_values = m_rows
                        | views::transform([](Food& food) -> auto& { return food.values; })
                        | views::join;
                    // clang-format on

                    const auto scale = value / prev_value;
                    for (auto&& other_value : views::concat(table_values, row_values)) {
                        other_value *= scale;
                    }
                }
            };

            add_row([&](auto&& add_cell) {
                add_cell(row_name);
                for (auto&& value : total_row.values) {
                    auto prev_value = value;
                    add_cell(row_value, value, prev_value);
                }
            });

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    } // Table for inputting meals


    { // Column of delete buttons
        ImGui::SameLine();
        ImGui::BeginGroup();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ FLT_MAX, 0.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.0f, 0.0f });

        // Use up the height of the table header
        ImGui::Dummy(ImVec2{ 0.0f, ImGui::GetTextLineHeightWithSpacing() });

        for (const auto& row_index : util::iota<int>(0, m_rows.size())) {
            ImGui::PushID(row_index);
            ImGui::TableNextColumn();

            if (ImGui::Button("x")) {
                m_rows.erase(m_rows.begin() + row_index);
            }
            ImGui::PopID();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndGroup();
    } // Column of delete buttons

    if (ImGui::ComboAutoSelect("Add food", data) && data.index != -1) {
        m_rows.push_back({
            .name = data.input,
        });
    }

    ImGui::End();
}
