#include "main_loop.hpp"
#include "resources.hpp"

constexpr u32 TEXTURE_WIDTH  = 1200;
constexpr u32 TEXTURE_HEIGHT = 800;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    Window window = CreateWindow("Fluid Simulation", 1200, 800).value();
    assert(window != nullptr);

    GPUDevice device = CreateGPUDevice().value();
    SDL_ClaimWindowForGPUDevice(device.get(), window.get());

    ImGuiHelper imgui(device.get(), window.get());

    MainLoop(MainLoopCallbacks{
        .on_event = [&](const SDL_Event& event) -> bool {
            return false;
        },
        .variable_update = [&](float dt) {
        },
        .fixed_update = [&](float dt) {},
        .render = [&]() {
            imgui.NewFrame();
            ImGui::Begin("Control panel");
            ImGui::End();
            imgui.EndFrame();

            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
            assert(command_buffer != nullptr);

            SDL_GPUTexture* swapchain_texture = nullptr;
            u32 swapchain_texture_width, swapchain_texture_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer, window.get(), &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height);

            if (swapchain_texture != nullptr) {
                SDL_GPUColorTargetInfo color_target = {
                    .texture     = swapchain_texture,
                    .clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f},
                    .load_op     = SDL_GPU_LOADOP_CLEAR,
                    .store_op    = SDL_GPU_STOREOP_STORE,
                };
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
                SDL_EndGPURenderPass(render_pass);

                imgui.DispatchRenderPass(command_buffer, swapchain_texture);
            }

            SDL_SubmitGPUCommandBuffer(command_buffer);
        }
    });

    SDL_ReleaseWindowFromGPUDevice(device.get(), window.get());

    return 0;
}

