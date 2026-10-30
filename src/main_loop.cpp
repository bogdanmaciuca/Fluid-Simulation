#include "main_loop.hpp"
#include "timer.hpp"
#include "ui.hpp"

void MainLoop(UpdateFunc variable_update, UpdateFunc fixed_update) {
    constexpr float MAX_FRAME_TIME  = 0.25f;
    constexpr int   MAX_FIXED_STEPS = 4;

    bool  running      = true;
    float t            = 0.0f;
    float dt           = FIXED_TIMESTEP;
    auto  current_time = std::chrono::high_resolution_clock::now();
    float accumulator  = 0.0f;

    while (running) {
        {
            Timer frame_timer("Frame");

            auto new_time = std::chrono::high_resolution_clock::now();
            float frame_time = std::chrono::duration<float>(new_time - current_time).count();
            current_time = new_time;
            frame_time = std::min(frame_time, MAX_FRAME_TIME);

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

            {
                Timer fixed_timer("Physics");

                // Never run an unbounded number of catch-up steps: if physics is slower
                // than real time, the backlog would otherwise grow every frame.
                int steps = 0;
                while (accumulator >= dt && steps < MAX_FIXED_STEPS) {
                    fixed_update(dt);
                    accumulator -= dt;
                    steps++;
                }

                // Drop stale time instead of trying to catch up forever.
                if (steps == MAX_FIXED_STEPS) {
                    accumulator = 0.0f;
                }
            }

            { Timer variable_timer("Render"); variable_update(frame_time); }
        }

        timers::EndFrame();
    }

}

