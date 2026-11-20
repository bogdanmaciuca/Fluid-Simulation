#include "main_loop.hpp"

#include "globals.hpp"
#include "helpers.hpp"

void MainLoop(const MainLoopCallbacks& callbacks) {
    auto& g = globals;

    bool  running      = true;
    float timestep     = FIXED_TIMESTEP;
    auto  current_time = std::chrono::high_resolution_clock::now();
    float accumulator  = 0.0f;

    while (running) {
        auto new_time = std::chrono::high_resolution_clock::now();
        float frame_time = std::chrono::duration<float>(new_time - current_time).count();
        current_time = new_time;

        for (auto& key : g.keys) {
            key.pressed = false;
            key.released = false;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Hand over event to ImGui
            ImGui_ImplSDL3_ProcessEvent(&event);
            // Don't process it ourselves if it was meant for ImGui
            if (ImGui::GetIO().WantCaptureMouse) continue;

            // User callback. Don't handle event here if user handled it already.
            if (callbacks.on_event(event)) continue;

            // Handle events not handled by ImGui or the user
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    g.keys[event.key.scancode].pressed = true;
                    g.keys[event.key.scancode].down = true;
                    break;
                case SDL_EVENT_KEY_UP:
                    g.keys[event.key.scancode].released = true;
                    g.keys[event.key.scancode].down = false;
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

            // Render
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(g.device);
            SDL_GPUTexture* swapchain_texture;
            u32 swapchain_texture_width, swapchain_texture_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, g.window, &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height);

            // Resize depth texture if needed
            static u32 last_swapchain_texture_width = swapchain_texture_width;
            static u32 last_swapchain_texture_height = swapchain_texture_height;
            if (last_swapchain_texture_width != swapchain_texture_width || last_swapchain_texture_height != swapchain_texture_height) {
                SDL_ReleaseGPUTexture(g.device, g.depth_texture);
                g.depth_texture = CreateDepthTexture(swapchain_texture_width, swapchain_texture_height);
            }

            if (swapchain_texture) {
                callbacks.render(command_buffer, swapchain_texture, swapchain_texture_width, swapchain_texture_height);
            }

            SDL_SubmitGPUCommandBuffer(command_buffer);
        }
    }
}

