#include "main_loop.hpp"
#include "ui.hpp"

void MainLoop(UpdateFunc variable_update, UpdateFunc fixed_update) {
    bool  running      = true;
    float t            = 0.0f;
    float dt           = FIXED_TIMESTEP;
    auto  current_time = std::chrono::high_resolution_clock::now();
    float accumulator  = 0.0f;

    while (running) {
        auto new_time = std::chrono::high_resolution_clock::now();
        float frame_time = std::chrono::duration<float>(new_time - current_time).count();
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

        accumulator += frame_time * g_ui.speed;
        while (accumulator >= dt ) {
            fixed_update(dt);
            accumulator -= dt;
        }

        variable_update(frame_time);
    }

}

