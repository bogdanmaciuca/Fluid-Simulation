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

    GPUTexture texture = CreateGPUTexture(device.get(), {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT,
        .usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE,
        .width                = TEXTURE_WIDTH,
        .height               = TEXTURE_HEIGHT,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
        .sample_count         = SDL_GPU_SAMPLECOUNT_1,
    }).value();

    GPUSampler sampler = CreateGPUSampler(device.get(), {
        .min_filter       = SDL_GPU_FILTER_LINEAR,
        .mag_filter       = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode      = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_v   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .address_mode_w   = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        .min_lod          = 0.0f,
        .max_lod          = 0.0f,
    }).value();

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

    std::array color_targets = {
        SDL_GPUColorTargetDescription{
            .format = SDL_GetGPUSwapchainTextureFormat(device.get(), window.get())
        }
    };
    GPUGraphicsPipelineEx gfx_pipeline(device.get(), "shaders/fullscreen.glsl", {
        .target_info = {
            .color_target_descriptions = color_targets.data(),
            .num_color_targets        = color_targets.size(),
        }
    });


    // the temporary variables modified by imgui
    glm::vec3 box_dimensions = { 1.0f, 1.0f, 1.0f };
    glm::vec3 gravity = { 0.0f, -9.81f, 0.0f };
    // the vectors
    struct Force{
    int id;
    int type = 0;

    glm::vec3 origin = { 0.0f, 0.0f, 0.0f };
    glm::vec3 direction = { 0.0f, 0.0f, 0.0f };

    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    float force = 0.0f;
    };
    std::vector<Force> forces;
    std::vector<int> ships;

    int next_force_id = 1, next_ship_id = 1;

    MainLoop(MainLoopCallbacks{
        .on_event = [&](const SDL_Event& event) -> bool {
            return false;
        },
        .variable_update = [&](float dt) {
            compute.ReloadIfModified();
            gfx_pipeline.ReloadIfModified();
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

                SDL_BindGPUGraphicsPipeline(render_pass, gfx_pipeline.Get());

                SDL_GPUTextureSamplerBinding sampler_binding = {
                    .texture = texture.get(),
                    .sampler = sampler.get(),
                };
                SDL_BindGPUFragmentSamplers(render_pass, 0, &sampler_binding, 1);
                SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
                SDL_EndGPURenderPass(render_pass);

                imgui.NewFrame();
                ImGui::Begin("Simulation Configuration");
                // BOX DIMENSIONS
                ImGui::SeparatorText("Box Dimensions");
                
                const float available_width = ImGui::GetContentRegionAvail().x;
                const float label_width = ImGui::CalcTextSize("X").x + ImGui::GetStyle().ItemSpacing.x;
                const float input_width = (available_width - (label_width * 3) - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;

                ImGui::AlignTextToFramePadding();
                ImGui::Text("X");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##BoxX", &box_dimensions.x);

                ImGui::SameLine();
                ImGui::Text("Y");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##BoxY", &box_dimensions.y);

                ImGui::SameLine();
                ImGui::Text("Z");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##BoxZ", &box_dimensions.z);

                // GRAVITY
                ImGui::SeparatorText("Gravity");

                ImGui::AlignTextToFramePadding();
                ImGui::Text("X");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##GravityX", &gravity.x);

                ImGui::SameLine();
                ImGui::Text("Y");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##GravityY", &gravity.y);

                ImGui::SameLine();
                ImGui::Text("Z");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(input_width);
                ImGui::InputFloat("##GravityZ", &gravity.z);

                // FORCES
                ImGui::SeparatorText("Forces");

                if (ImGui::Button("+ Add Force"))
                {
                    forces.push_back({next_force_id++});
                }

                for (int i = 0; i < forces.size(); i++)
                {
                    ImGui::PushID(forces[i].id);

                    ImGui::Text("Force %d", forces[i].id);

                    const char* forceTypes[] = { "Unassigned", "Directional", "Point", "Vortex" };

                    ImGui::SameLine();
                    ImGui::Combo("Type", &forces[i].type, forceTypes, IM_ARRAYSIZE(forceTypes));

                    ImGui::SameLine();
                    if (ImGui::Button("X"))
                    {
                        forces.erase(forces.begin() + i);
                        ImGui::PopID();
                        break;
                    }

                    switch (forces[i].type){
                            case 0: // Unassigned
                            {
                                break;
                            }

                            case 1: // Directional
                            {
                                ImGui::Indent();

                                ImGui::Text("Origin");
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("X##OriginX", &forces[i].origin.x);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Y##OriginY", &forces[i].origin.y);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Z##OriginZ", &forces[i].origin.z);

                                ImGui::Text("Direction");
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("X##DirectionX", &forces[i].direction.x);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Y##DirectionY", &forces[i].direction.y);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Z##DirectionZ", &forces[i].direction.z);

                                ImGui::Unindent();
                                break;
                            }

                            case 2: // Point
                            {
                                ImGui::Indent();

                                ImGui::Text("Position");
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("X##PositionX", &forces[i].position.x);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Y##PositionY", &forces[i].position.y);
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(100.0f);
                                ImGui::InputFloat("Z##PositionZ", &forces[i].position.z);

                                ImGui::Text("Force");
                                ImGui::SetNextItemWidth(150.0f);
                                ImGui::InputFloat("##PointForce", &forces[i].force);

                                ImGui::Unindent();
                                break;
                            }
                        }

                    ImGui::PopID();
                }

                // SHIPS
                ImGui::SeparatorText("Ships");

                if (ImGui::Button("+ Add Ship"))
                {
                    ships.push_back(next_ship_id++);
                }

                for (int i = 0; i < ships.size(); i++)
                {
                    ImGui::PushID(ships[i]);

                    ImGui::Text("Ship %d", ships[i]);

                    ImGui::SameLine();
                    if (ImGui::Button("X"))
                    {
                        ships.erase(ships.begin() + i);
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }

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

