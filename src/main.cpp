#include "renderer.hpp"

int main() {
    RendererInitialize();

    bool  running      = true;
    float t            = 0.0f;
    float dt           = 0.01666f; // 60Hz
    auto  current_time = std::chrono::high_resolution_clock::now();
    float accumulator  = 0.0f;

    while (running) {
        auto new_time = std::chrono::high_resolution_clock::now();
        float frame_time = std::chrono::duration<float, std::milli>(new_time - current_time).count();
        current_time = new_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        accumulator += frame_time;
        while (accumulator >= dt) {
            // RunPhysicsStep(dt);
            accumulator -= dt;
        }

        RendererDrawFrame();
    }

    RendererShutdown();
    return 0;
}

