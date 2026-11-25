#include "main_loop.hpp"
#include "resources.hpp"

constexpr u32 DEFAULT_WND_W = 1200;
constexpr u32 DEFAULT_WND_H = 800;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    Window window = CreateWindow("Fluid Simulation", DEFAULT_WND_W, DEFAULT_WND_H).value();
    assert(window != nullptr);

    GPUDevice device = CreateGPUDevice().value();
    SDL_ClaimWindowForGPUDevice(device.get(), window.get());

    ImGuiHelper imgui(device.get(), window.get());

    GPUTextureEx scene_texture(device.get(), {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GetGPUSwapchainTextureFormat(device.get(), window.get()),
        .usage                = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width                = DEFAULT_WND_W,
        .height               = DEFAULT_WND_H,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
    });

    MainLoop(MainLoopCallbacks{
        .on_event = [&](const SDL_Event& event) -> bool {
            return false;
        },
        .fixed_update = [&](float dt) {
        },
        .variable_update = [&](float dt) {
            imgui.ControlPanelAndScene(
                scene_texture,
                [&](float viewport_width, float viewport_height) {
                    ImGui::Text("Hello World!");
                    scene_texture.Resize(viewport_width, viewport_height);
                }
            );

            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
            assert(command_buffer != nullptr);

            SDL_GPUTexture* swapchain_texture = nullptr;
            u32 swapchain_texture_width, swapchain_texture_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer, window.get(), &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height);

            if (swapchain_texture != nullptr) {
                SDL_GPUColorTargetInfo color_target = {
                    .texture     = scene_texture.Get(),
                    .clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f},
                    .load_op     = SDL_GPU_LOADOP_CLEAR,
                    .store_op    = SDL_GPU_STOREOP_STORE,
                };
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);
                SDL_EndGPURenderPass(render_pass);

                imgui.DispatchRenderPass(command_buffer, swapchain_texture, true);
            }

            SDL_SubmitGPUCommandBuffer(command_buffer);
        },
    });

    SDL_ReleaseWindowFromGPUDevice(device.get(), window.get());

    return 0;
}

