#include "main_loop.hpp"
#include "resources.hpp"

constexpr u32 TEXTURE_WIDTH  = 1200;
constexpr u32 TEXTURE_HEIGHT = 800;

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    Window window = CreateWindow("Fluid Simulation", 1200, 800);
    assert(window != nullptr);

    GPUDevice device = CreateGPUDevice();
    SDL_ClaimWindowForGPUDevice(device.get(), window.get());

    ImGuiHelper imgui(device.get(), window.get());

    GPUTexture texture = CreateGPUTexture(device.get(), {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE,
        .width                = TEXTURE_WIDTH,
        .height               = TEXTURE_HEIGHT,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
        .sample_count         = SDL_GPU_SAMPLECOUNT_1,
    });

    GPUSampler sampler = CreateGPUSampler(device.get(), {
        .min_filter       = SDL_GPU_FILTER_LINEAR,
        .mag_filter       = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode      = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .min_lod          = 0.0f,
        .max_lod          = 0.0f,
    });

    GPUComputePipelineEx compute(
        device.get(),
        "shaders/compute.comp",
        {{"VALUE", "1.0"}}
    );
    compute.SetBindings(
        std::array{
            SDL_GPUStorageTextureReadWriteBinding{
                .texture = texture.get(),
                .mip_level = 0,
                .layer = 0,
                .cycle = false,
            }
        },
        {}
    );

    GPUShader vs = CreateGPUShaderVertex(device.get(), "shaders/fullscreen.glsl");
    GPUShader fs = CreateGPUShaderFragment(device.get(), "shaders/fullscreen.glsl");

    std::array color_targets = {
        SDL_GPUColorTargetDescription{
            .format = SDL_GetGPUSwapchainTextureFormat(device.get(), window.get())
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo gfx_info = {
        .vertex_shader = vs.get(),
        .fragment_shader = fs.get(),
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },
        .multisample_state = {
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
        },
        .target_info = {
            .color_target_descriptions = color_targets.data(),
            .num_color_targets        = color_targets.size(),
            .has_depth_stencil_target = false,
        }
    };
    GPUGraphicsPipeline gfx_pipeline = CreateGPUGraphicsPipeline(device.get(), gfx_info);

    MainLoop(MainLoopCallbacks{
        .on_event = [&](const SDL_Event& event) -> bool {
            return false;
        },
        .variable_update = [&](float dt) {
            compute.ReloadIfModified();
        },
        .fixed_update = [&](float dt) {},
        .render = [&]() {
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());
            assert(command_buffer != nullptr);

            SDL_GPUTexture* swapchain_texture = nullptr;
            u32 swapchain_texture_width, swapchain_texture_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(
                command_buffer, window.get(), &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height);

            if (swapchain_texture != nullptr) {
                compute.DispatchAsPass(command_buffer, TEXTURE_WIDTH, TEXTURE_HEIGHT, 1);

                SDL_GPUColorTargetInfo color_target = {
                    .texture     = swapchain_texture,
                    .clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f},
                    .load_op     = SDL_GPU_LOADOP_CLEAR,
                    .store_op    = SDL_GPU_STOREOP_STORE,
                };
                SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, nullptr);

                SDL_BindGPUGraphicsPipeline(render_pass, gfx_pipeline.get());

                SDL_GPUTextureSamplerBinding sampler_binding = {
                    .texture = texture.get(),
                    .sampler = sampler.get(),
                };
                SDL_BindGPUFragmentSamplers(render_pass, 0, &sampler_binding, 1);

                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

                SDL_EndGPURenderPass(render_pass);

                imgui.NewFrame();
                ImGui::Begin("Hello byatch");
                ImGui::End();
                imgui.EndFrame();

                imgui.DispatchRenderPass(command_buffer, swapchain_texture);
            }

            SDL_SubmitGPUCommandBuffer(command_buffer);
        }
    });

    SDL_ReleaseWindowFromGPUDevice(device.get(), window.get());

    return 0;
}

