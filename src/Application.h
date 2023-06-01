#pragma once
#include <SDL_video.h>
#include <memory>


class Application {
public:
    Application();
    virtual ~Application();

    void run();
    static bool is_initialized() {
        return s_initialized;
    }

protected:
    virtual void on_update(double dt) = 0;
    SDL_Window* get_window() const {
        return m_window;
    }

private:
    SDL_Window* m_window             = nullptr;
    SDL_GLContext m_context          = nullptr;
    inline static bool s_initialized = false;
};

std::unique_ptr<Application> create_application();
