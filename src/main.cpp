#include "helpers.hpp"
#include "main_loop.hpp"
#include "renderer/renderer.hpp"
#include "util.hpp"

constexpr u32 DEFAULT_WND_W = 1200;
constexpr u32 DEFAULT_WND_H = 800;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    Window window = CreateWindow("Fluid Simulation", DEFAULT_WND_W, DEFAULT_WND_H).value();
    assert(window != nullptr);

    GPUDevice device = CreateGPUDevice().value();
    SDL_ClaimWindowForGPUDevice(device.get(), window.get());

    const bool* keys = SDL_GetKeyboardState(nullptr);

    ImGuiHelper imgui(device.get(), window.get());
    auto& io = ImGui::GetIO();

    GPUTextureEx scene_texture(device.get(), {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GetGPUSwapchainTextureFormat(device.get(), window.get()),
        .usage                = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width                = DEFAULT_WND_W,
        .height               = DEFAULT_WND_H,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
    });

    Renderer renderer(device.get(), scene_texture.GetCreateInfo().format, nullptr);

    bool scene_is_focused = true;
    MainLoop(MainLoopCallbacks{
        .on_event = [&](const SDL_Event& event) -> bool {
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                renderer.ProcessMouseScroll(event.wheel.y);
            }
            return false;
        },
        .fixed_update = [&](float dt) {
        },
        .variable_update = [&](float dt) {
            imgui.ControlPanelAndScene(
                scene_texture,
                [&]() {
                    ImGui::Text("Hello World!");
                },
                &scene_is_focused
            );

            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
            assert(command_buffer != nullptr);

            SDL_GPUTexture* swapchain_texture = nullptr;
            u32 swapchain_texture_width, swapchain_texture_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer, window.get(), &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height);

            if (swapchain_texture != nullptr) {
                renderer.ProcessCameraInput([&](Camera& cam) {
                    if (scene_is_focused) {
                        // Mouse
                        vec2 delta = GetMouseDelta();
                        if (LeftMouseButtonDown()) {
                            cam.ProcessMouseMovement(delta.x, -delta.y);
                        }

                        // Keyboard
                        if (keys[SDL_SCANCODE_W]) cam.ProcessKeyboard(CameraMovement::Forward, dt);
                        else if (keys[SDL_SCANCODE_S]) cam.ProcessKeyboard(CameraMovement::Backward, dt);
                        if (keys[SDL_SCANCODE_A]) cam.ProcessKeyboard(CameraMovement::Left, dt);
                        else if (keys[SDL_SCANCODE_D]) cam.ProcessKeyboard(CameraMovement::Right, dt);

                    }
                });

                renderer.PushCone(Cone{.origin = vec3(0, 0, 0), .dir = vec3(0, 1, 0), .radius = 2.0f});
                renderer.PushSphere(Sphere{.origin = vec3(1.5f, 1.0f, -3.0f), .radius = 0.75f});
                renderer.PushSphere(Sphere{.origin = vec3(-2.0f, 0.5f, -4.5f), .radius = 0.4f});
                renderer.PushCylinder(Cylinder{.origin = vec3(-1.0f, 0.0f, -2.0f), .dir = vec3(0.0f, 1.5f, 0.0f), .radius = 0.5f});
                renderer.PushCylinder(Cylinder{.origin = vec3(2.5f, 0.0f, -5.0f), .dir = vec3(0.5f, 1.0f, 0.0f), .radius = 0.3f});
                renderer.PushCone(Cone{.origin = vec3(-3.0f, 0.0f, -6.0f), .dir = vec3(0.0f, 2.0f, 0.0f), .radius = 0.8f});
                renderer.PushCone(Cone{.origin = vec3(3.0f, 0.0f, -1.5f), .dir = vec3(1.0f, 1.0f, 0.0f), .radius = 0.5f});
                renderer.PushAABB(AABB{.min = vec3(0.5f, 0.0f, -7.0f), .max = vec3(1.5f, 1.0f, -6.0f)});
                renderer.PushAABB(AABB{.min = vec3(-1.5f, 0.0f, -8.0f), .max = vec3(-0.5f, 2.0f, -7.0f)});
                renderer.RenderFrame(
                    command_buffer,
                    scene_texture.Get(),
                    scene_texture.GetCreateInfo().width,
                    scene_texture.GetCreateInfo().height
                );
                imgui.DispatchRenderPass(command_buffer, swapchain_texture, false);
            }

            SDL_SubmitGPUCommandBuffer(command_buffer);
        },
    });

    SDL_ReleaseWindowFromGPUDevice(device.get(), window.get());

    return 0;
}
