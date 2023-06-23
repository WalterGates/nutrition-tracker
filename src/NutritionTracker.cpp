#include "NutritionTracker.h"

#include <fstream>
#include <fmt/format.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <range/v3/view/enumerate.hpp>
#include "imgui_combo_autoselect.h"


// TODO: Fix `.clang-format` as to not have to override clang-format
// clang-format off
EditMealWidget::EditMealWidget(const std::shared_ptr<food_values_table_type>& food_values_table) {
    const auto food_names = *food_values_table | views::transform([](auto&& pair) {
        return std::forward<decltype(pair.first)>(pair.first);
    });

    m_food_values_table = food_values_table;
    m_dropdown_data     = ImGui::ComboAutoSelectData{ { food_names.begin(), food_names.end() } };
}

EditMealWidget::EditMealWidget(const json& json_serial, const std::shared_ptr<food_values_table_type>& food_values_table)
    : EditMealWidget(food_values_table)
{
    deserialize(json_serial);
}
// clang-format on

json EditMealWidget::serialize() {
    auto result = json{};
    for (const auto& row : m_rows) {
        result["rows"].push_back(row);
    }

    result["title"] = m_title;
    result["notes"] = m_notes;
    return result;
}

EditMealWidget& EditMealWidget::deserialize(const json& json_serial) {
    if (json_serial.contains("rows")) {
        m_rows.clear();
        m_rows.reserve(json_serial.at("rows").size());

        for (const auto& row : json_serial["rows"]) {
            m_rows.push_back(row.get<Food>());
        }
        recalculate_total();
    }

    if (json_serial.contains("title")) {
        json_serial["title"].get_to(m_title);
    }

    if (json_serial.contains("notes")) {
        json_serial["notes"].get_to(m_notes);
    }

    return *this;
}

void EditMealWidget::draw() {
    draw_text_input();
    ImGui::Separator();
    ImGui::NewLine();

    draw_table();

    ImGui::SameLine();
    ImGui::BeginGroup();
    draw_remove_buttons();
    ImGui::EndGroup();

    draw_add_food_dropdown();

    if (ImGui::Button("Save")) {
        std::ofstream("res/day0.json") << serialize();
    }
}

// clang-format off
static bool InputText(const std::string_view label, const std::string_view hint, std::string& buffer, const ImVec2 size = {},
    const ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr)
{
    struct UserData {
        std::string* str                      = nullptr;
        ImGuiInputTextCallback chain_callback = nullptr;
        void* chain_callback_user_data        = nullptr;
    };

    constexpr auto resize_callback = [](ImGuiInputTextCallbackData* data) {
        auto* user_data = static_cast<UserData*>(data->UserData);

        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            auto* str = user_data->str;
            IM_ASSERT(data->Buf == str->c_str());
            str->resize(static_cast<size_t>(data->BufTextLen));
            data->Buf = str->data();

        } else if (user_data->chain_callback != nullptr) {
            data->UserData = user_data->chain_callback_user_data;
            return user_data->chain_callback(data);
        }
        return 0;
    };

    auto resize_user_data = UserData{ .str = &buffer, .chain_callback = callback, .chain_callback_user_data = user_data };

    // FIXME: Pressing escape clears the text input to the state it was in when it was last focused. This is incredibly
    // frustrating for vim users who instinctively press escape whenever they are done typing something
    return ImGui::InputTextEx(label.data(), hint.data(), buffer.data(), static_cast<int>(buffer.size()), size,
        flags | ImGuiInputTextFlags_CallbackResize, resize_callback, &resize_user_data);
}

static bool InputText(const std::string_view label, std::string& buffer, const ImVec2 size = {},
    const ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr)
{
    return InputText(label, "", buffer, size, flags, callback, user_data);
}
// clang-format on

void EditMealWidget::draw_text_input() {
    const auto screen_width    = ImGui::GetContentRegionAvail().x;
    const auto min_input_width = ImGui::CalcTextSize("a").x * 30;
    const auto input_width     = std::max(std::min(min_input_width, screen_width), screen_width * 0.5f);

    // TODO: Change the title hint to the auto-generated title based on the time that will be used
    // if the user leaves this text input empty.
    InputText("##meal_title", "Enter the title of this meal", m_title, ImVec2{ input_width, 0 });
    InputText("##meal_notes", "Enter notes about this meal", m_notes, ImVec2{ input_width, ImGui::GetFontSize() * 6 },
        ImGuiInputTextFlags_Multiline);
}

void EditMealWidget::draw_table() {
    constexpr auto column_names = std::array{ "Food"sv, "Protein"sv, "Carbo"sv, "Fat"sv, "Calories"sv, "Weight"sv };
    constexpr auto table_flags  = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.0f, 0.0f });

    // TODO: Make table sortable and scrollable
    if (ImGui::BeginTable("##input_table", column_names.size(), table_flags)) {
        for (const auto& name : column_names) {
            ImGui::TableSetupColumn(name.data());
        }

        ImGui::TableHeadersRow();
        reset_ids();

        bool table_edited = false;
        for (auto&& row : m_rows) {
            table_edited |= draw_value_row(row);
        }

        if (table_edited) {
            recalculate_total();
        }

        draw_total_row();
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

void EditMealWidget::draw_remove_buttons() {
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
}

void EditMealWidget::draw_add_food_dropdown() {
    if (ImGui::ComboAutoSelect("Add food", m_dropdown_data) && m_dropdown_data.index != -1) {
        m_rows.push_back({
            .name = m_dropdown_data.input,
        });
    }
}

bool EditMealWidget::draw_value_row(Food& row) {
    bool table_edited = false;

    next_column([&] {
        if (ImGui::ComboAutoSelect("##food_dropdown", m_dropdown_data) && m_dropdown_data.index != -1) {
            ranges::fill(row.values, 0.0f);
            table_edited |= true;
        }
    });

    for (auto&& [index, value] : row.values | views::enumerate) {
        next_column([&] {
            if (ImGui::DragFloat("##grams_input", &value, 1.0f, 0.0f, 10'000.0f, "%.1fg")) {
                const auto& food_props = (*m_food_values_table)[row.name];
                const auto weight      = food_props.get_weight_from_value(index, value);

                // clang-format off
                auto other_values = row.values
                    | views::enumerate
                    | views::filter([&](const auto& pair) { return &value != &pair.second; });
                // clang-format on

                for (auto&& [other_index, other_value] : other_values) {
                    other_value = food_props.get_value_from_weight(other_index, weight);
                }
                table_edited |= true;
            }
        });
    }

    return table_edited;
}

void EditMealWidget::draw_total_row() {
    // TODO: Make this row be fixed when adding scrolling to the table. Maybe even make it a different color

    next_column([&] {
        // HACK: `ImGui::InputText` looks the nicest but I don't like the fact that i can select it.
        // That on its own isn't all that bad but I hate seeing the cursor blinking. I think
        // `ImGui::Text` should be the best option if I can set the background to be blue.

        // ImGui::TextUnformatted(total_row.name.c_str());
        // ImGui::Selectable(total_row.name.c_str(), true);
        ImGui::InputText("##total_row_name", m_total_row.name.data(), m_total_row.name.size(), ImGuiInputTextFlags_ReadOnly);
    });

    for (auto&& value : m_total_row.values) {
        next_column([&] {
            // HACK: ImGui::DragFloat is triggered on every input change. This is the desired behavior for drag
            // input but for key input, if you would enter something like `0.5`, this would be triggered first
            // for just the `0` and thus it would multiply the whole table by 0.
            // For now I fixed this by forcing a minimum input value, but this still could end up multiplying
            // by a small value then by something really small then by something bigger, thus loosing precision.

            auto prev_value = value;
            if (ImGui::DragFloat("##grams_input", &value, 1.0f, 0.1f, 10'000.0f, "%.1fg")) {
                // Guard for division by zero
                if (std::abs(prev_value) < std::numeric_limits<float>::epsilon()) {
                    value = 0.0f;
                    return;
                }

                // clang-format off
                auto row_values = m_total_row.values
                    | views::filter([&](const auto& other_value) { return &value != &other_value; });

                auto table_values = m_rows
                    | views::transform([](Food& food) -> auto& { return food.values; })
                    | views::join;
                // clang-format on

                for (auto&& other_value : views::concat(table_values, row_values)) {
                    other_value *= value / prev_value;
                }
            }
        });
    }
}

void EditMealWidget::reset_ids() {
    m_next_id = 0;
}

void EditMealWidget::next_column(auto&& func) {
    ImGui::TableNextColumn();
    ImGui::PushID(m_next_id++);
    ImGui::SetNextItemWidth(-FLT_MIN);

    func();
    ImGui::PopID();
}

void EditMealWidget::recalculate_total() {
    ranges::fill(m_total_row.values, 0.0f);

    for (const auto& row : m_rows) {
        for (const auto& index : views::iota(0u, row.values.size())) {
            m_total_row.values[index] += row.values[index];
        }
    }
}

NutritionTracker::NutritionTracker() {
    auto food_json = json::parse(std::ifstream{ "res/database.json" });

    for (const auto& [food_name, json_props] : food_json.items()) {
        auto is_valid   = (json_props.size() == Food::value_names.size());
        auto food_props = FoodProps{};

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

        m_food_values_table->emplace(food_name, food_props);
    }

    // NOTE: The `EditMealWidget` constructor needs the finalized food hash table because it creates a copy of it's keys
    m_edit_meal_widget = EditMealWidget{ json::parse(std::ifstream{ "res/day0.json" }), m_food_values_table };
}

void NutritionTracker::on_update(double /*dt*/) {
    // TODO: Make windows pop out
    // TODO: Add DPI awareness
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Begin("Meal Window");
    m_edit_meal_widget.draw();
    ImGui::End();
}

std::unique_ptr<Application> create_application() {
    return std::make_unique<NutritionTracker>();
}
