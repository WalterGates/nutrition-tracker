set(sources
    ./Application.cpp
    ./NutritionTracker.cpp
    ./imgui_combo_autoselect.cpp
)

set(headers
    ./Application.h
    ./NutritionTracker.h
    ./imgui_combo_autoselect.h
)

add_executable(main ${sources} ${headers})
target_link_libraries(main PRIVATE project_options project_warnings)

# find and link dependencies
target_find_dependencies(main PRIVATE_CONFIG fmt imgui SDL2 range-v3 nlohmann_json)
target_link_system_libraries(main PRIVATE
    fmt::fmt imgui::imgui SDL2::SDL2 range-v3::range-v3 nlohmann_json::nlohmann_json GL
)

# Package the project
package_project(TARGETS main)
