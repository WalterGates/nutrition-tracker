#include <SDL.h>
#include <SDL_opengl.h>

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui.h>

#include <fmt/format.h>
#include <fmt/color.h>
#include <chrono>
#include <imgui_internal.h>
#include <ratio>

#include "Application.h"
#include "Utils.h"


Application::Application() {
    if (Application::is_initialized()) {
        fmt::print(stderr, fmt::fg(fmt::color::red), "[ERROR] Attempted to initialize the app more than once!\n");
        return;
    }

    // SDL2 window and OpenGL context creating
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        fmt::print(stderr, fmt::fg(fmt::color::red), "[ERROR] {}\n", SDL_GetError());
        return;
    }

    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window with graphics context
    m_window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (m_window == nullptr) {
        fmt::print(stderr, fmt::fg(fmt::color::red), "[ERROR] Failed to create window!\n");
    }

    m_context = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1);

    // Setting up Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    const auto latin_ranges = std::array<ImWchar, 7>{ 0x0020, 0x007E, // Basic Latin range
        0x00A1, 0x024F,                                               // Latin-1 Supplement range
        0x0100, 0x0173,                                               // Latin Extended-A
        // 0x0180, 0x024f, // Latin Extended-B
        0 };

    const auto font = "res/CascadiaCode-Regular.otf"sv;
    if (io.Fonts->AddFontFromFileTTF(font.data(), 20.0f, nullptr, latin_ranges.data()) == nullptr) {
        fmt::print(fmt::fg(fmt::color::red), "[ERROR]: Font '{}' not found", font.data());
    }

    io.Fonts->Build();

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(1.5f);

    ImGui_ImplSDL2_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    s_initialized = true;
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(m_context);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
    s_initialized = false;
}

void Application::run() {
    auto then = std::chrono::high_resolution_clock::now();
    auto& io  = ImGui::GetIO();

    for (auto running = true; running;) {
        for (auto event = SDL_Event{}; SDL_PollEvent(&event) != 0;) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            // clang-format off
            running = (event.type != SDL_QUIT && (
                event.type != SDL_WINDOWEVENT ||
                event.window.event != SDL_WINDOWEVENT_CLOSE ||
                event.window.windowID != SDL_GetWindowID(m_window)
            ));
            //clang-format on
        }

        // Begin new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        const auto now          = std::chrono::high_resolution_clock::now();
        const auto elapsed_time = std::chrono::duration<double>{ now - then };
        on_update(elapsed_time.count());
        then = now;

        // End frame
        ImGui::Render();
        glViewport(0, 0, static_cast<GLint>(io.DisplaySize.x), static_cast<GLint>(io.DisplaySize.y));
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(m_window);
    }
}

int main(int /*argc*/, char* /*argv*/[]) {
    if (auto app = create_application(); app->is_initialized()) {
        app->run();
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
