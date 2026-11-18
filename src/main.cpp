#include "camera.hpp"
#include "globals.hpp"
#include "lifecycle.hpp"
#include "main_loop.hpp"

int main() {
    auto& g = globals;

    Initialize();

    Camera camera;

    MainLoop({
        .on_event = [&](const SDL_Event& event) -> bool {
            switch (event.type) {
                case SDL_EVENT_MOUSE_WHEEL:
                    camera.ProcessMouseScroll(event.wheel.y);
                    return true;
            }
            return false;
        },

        .variable_update = [&](float delta_time) {
            // Process mouse movement for camera only if LMB is down
            float dx, dy;
            SDL_MouseButtonFlags mouse_flags = SDL_GetRelativeMouseState(&dx, &dy);
            if (mouse_flags & SDL_BUTTON_LMASK) {
                camera.ProcessMouseMovement(dx, -dy);
            }

            // Process keyboard
            if (g.keys[SDL_SCANCODE_W].down) {
                camera.ProcessKeyboard(CameraMovement::Forward, delta_time);
            }
            else if (g.keys[SDL_SCANCODE_S].down) {
                camera.ProcessKeyboard(CameraMovement::Backward, delta_time);
            }
            if (g.keys[SDL_SCANCODE_A].down) {
                camera.ProcessKeyboard(CameraMovement::Left, delta_time);
            }
            else if (g.keys[SDL_SCANCODE_D].down) {
                camera.ProcessKeyboard(CameraMovement::Right, delta_time);
            }
        },

        .fixed_update = [&](float timestep) {
        },

        .render = [&](
            SDL_GPUCommandBuffer* command_buffer,
            SDL_GPUTexture* swapchain_texture,
            u32 swapchain_texture_width, u32 swapchain_texture_height
        ) {
            SDL_GPUColorTargetInfo color_target = {
                .texture = swapchain_texture,
                .clear_color = SDL_FColor{0.7f, 0.1f, 0.5f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
                .cycle = true,
            };
            SDL_GPUDepthStencilTargetInfo depth_stencil_target = {
                .texture          = g.depth_texture,
                .clear_depth      = 1.0f,
                .load_op          = SDL_GPU_LOADOP_CLEAR,
                .store_op         = SDL_GPU_STOREOP_STORE,
                .stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE,
                .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE
            };
            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
            SDL_EndGPURenderPass(render_pass);
        },
    });

    Shutdown();

    return 0;
}

