#include "main_loop.hpp"

void MainLoop(const MainLoopCallbacks& callbacks) {
    bool  running      = true;
    float timestep     = FIXED_TIMESTEP;
    auto  current_time = std::chrono::high_resolution_clock::now();
    float accumulator  = 0.0f;

    while (running) {
        auto new_time = std::chrono::high_resolution_clock::now();
        float frame_time = std::chrono::duration<float>(new_time - current_time).count();
        current_time = new_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Hand over event to ImGui
            // ImGui_ImplSDL3_ProcessEvent(&event);
            // Don't process it ourselves if it was meant for ImGui
            // if (ImGui::GetIO().WantCaptureMouse) continue;

            // User callback. Don't handle event here if user handled it already.
            if (callbacks.on_event(event)) continue;

            // Handle events not handled by ImGui or the user
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
            }
        }

        accumulator += frame_time;
        while (accumulator >= timestep) {
            if (callbacks.fixed_update)
                callbacks.fixed_update(timestep);
            accumulator -= timestep;
        }

        if (callbacks.variable_update) {
            callbacks.variable_update(frame_time);
            callbacks.render();
        }
    }
}

