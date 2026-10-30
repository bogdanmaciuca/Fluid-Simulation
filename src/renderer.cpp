#include "renderer.hpp"
#include "ui.hpp"

constexpr const char* SPHERES_VERTEX_SHADER_PATH   = "shaders/spheres.vert";
constexpr const char* SPHERES_FRAGMENT_SHADER_PATH = "shaders/spheres.frag";

constexpr const char* BOX_VERTEX_SHADER_PATH   = "shaders/box.vert";
constexpr const char* BOX_FRAGMENT_SHADER_PATH = "shaders/box.frag";

constexpr uint32_t BUFFER_SLOT_POSITIONS = 0;
constexpr uint32_t BUFFER_SLOT_COLORS    = 1;

namespace {
    SDL_Window*              s_window;
    SDL_GPUDevice*           s_device;
    SDL_GPUGraphicsPipeline* s_spheres_pipeline;
    SDL_GPUGraphicsPipeline* s_box_pipeline;
    SDL_GPUTexture*          s_depth_texture;

    SDL_GPUTransferBuffer*   s_transfer_buffer;
    size_t                   s_transfer_buffer_size;

    SDL_GPUBuffer*           s_sphere_positions;
    SDL_GPUBuffer*           s_sphere_colors;
    size_t                   s_sphere_buffers_size;
    size_t                   s_spheres_num;

    ImGuiIO*                 s_io;
    std::function<void()>    s_imgui_func;

    struct PerFrameUBOSpheres {
        glm::mat4 view_proj;
        float     aspect_ratio;
        float     radius;
    };
    glm::mat4 s_view = glm::mat4(1);

    struct PerFrameUBOBox {
        glm::mat4 view_proj;
        glm::vec2 screen_resolution;
        float     line_width;
    };

    bool EnsureTransferBufferSize(size_t requested_size) {
        if (requested_size > s_transfer_buffer_size) {
            SDL_GPUTransferBufferCreateInfo create_info = {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size  = static_cast<uint32_t>(requested_size * 2)
            };
            auto new_transfer_buffer = SDL_CreateGPUTransferBuffer(s_device, &create_info);
            if (!new_transfer_buffer) {
                printf("Failed to create transfer buffer: %s", SDL_GetError());
                return false;
            }
            else {
                SDL_ReleaseGPUTransferBuffer(s_device, s_transfer_buffer);
                s_transfer_buffer = new_transfer_buffer;
                s_transfer_buffer_size = requested_size * 2;
            }
        }
        return true;
    }

    bool EnsureSpheresBufferSize(size_t requested_size) {
        if (requested_size > s_sphere_buffers_size) {
            SDL_GPUBufferCreateInfo create_info = {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size  = static_cast<uint32_t>(requested_size * 2)
            };
            auto new_positions = SDL_CreateGPUBuffer(s_device, &create_info);
            if (!new_positions) {
                printf("Failed to create positions buffer: %s", SDL_GetError());
                return false;
            }
            auto new_colors = SDL_CreateGPUBuffer(s_device, &create_info);
            if (!new_colors) {
                printf("Failed to create colors buffer: %s", SDL_GetError());
                return false;
            }
            SDL_ReleaseGPUBuffer(s_device, s_sphere_positions);
            SDL_ReleaseGPUBuffer(s_device, s_sphere_colors);
            s_sphere_positions = new_positions;
            s_sphere_colors = new_colors;
            s_sphere_buffers_size = requested_size * 2;
        }
        return true;
    }

    bool UploadSphereData(
        SDL_GPUCommandBuffer* command_buffer,
        std::span<const glm::vec3> sphere_positions,
        std::span<const glm::vec3> sphere_colors
    ) {
        assert(sphere_positions.size() == sphere_colors.size());
        if(sphere_positions.empty()) {
            return true;
        }

        // Ensure GPU buffer sizes
        if (!EnsureTransferBufferSize(sphere_positions.size_bytes() + sphere_colors.size_bytes())) return false;
        if (!EnsureSpheresBufferSize(sphere_positions.size_bytes())) return false;

        s_spheres_num = sphere_positions.size();

        // Populate transfer buffer with positions followed by colors
        void* mapped_mem = SDL_MapGPUTransferBuffer(s_device, s_transfer_buffer, false);
        // Positions
        memcpy(mapped_mem, sphere_positions.data(), sphere_positions.size_bytes());
        // Colors
        memcpy(static_cast<std::byte*>(mapped_mem) + sphere_positions.size_bytes(), sphere_colors.data(), sphere_colors.size_bytes());
        SDL_UnmapGPUTransferBuffer(s_device, s_transfer_buffer);

        // Record the CPU-side copy pass. Actual VRAM transfer is asynchronous.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        // Positions
        SDL_GPUTransferBufferLocation positions_source = {
            .transfer_buffer = s_transfer_buffer,
            .offset = 0
        };
        SDL_GPUBufferRegion positions_destination = {
            .buffer = s_sphere_positions,
            .offset = 0,
            .size = static_cast<uint32_t>(sphere_positions.size_bytes())
        };
        SDL_UploadToGPUBuffer(copy_pass, &positions_source, &positions_destination, false);
        // Colors
        SDL_GPUTransferBufferLocation colors_source = {
            .transfer_buffer = s_transfer_buffer,
            .offset = static_cast<uint32_t>(sphere_positions.size_bytes())
        };
        SDL_GPUBufferRegion colors_destination = {
            .buffer = s_sphere_colors,
            .offset = 0,
            .size = static_cast<uint32_t>(sphere_colors.size_bytes())
        };
        SDL_UploadToGPUBuffer(copy_pass, &colors_source, &colors_destination, false);
        SDL_EndGPUCopyPass(copy_pass);

        return true;
    }

    bool CompileShader(const char* path, EShLanguage stage, std::vector<uint32_t>& result) {
        result.clear();

        char* shader_source = static_cast<char*>(SDL_LoadFile(path, nullptr));
        if (!shader_source) {
            SDL_Log("Could not load file '%s': %s", path, SDL_GetError());
            return false;
        }

        glslang::TShader shader(stage);
        shader.setStrings(&shader_source, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

        EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
        if (!shader.parse(GetDefaultResources(), 100, false, messages)) {
            std::cerr << "GLSL Compilation Failed (" << (stage == EShLangVertex ? "Vertex" : "Fragment") << "):\n"
                << shader.getInfoLog() << "\n"
                << shader.getInfoDebugLog() << std::endl;
            return false;
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(messages)) {
            std::cerr << "GLSL Linking Failed:\n"
                << program.getInfoLog() << "\n"
                << program.getInfoDebugLog() << std::endl;
            return false;
        }

        glslang::GlslangToSpv(*program.getIntermediate(stage), result);

        SDL_free(shader_source);
        return true;
    }

    bool InitializeSpheresPipeline() {
        // Vertex shader
        std::vector<uint32_t> vertex_shader_spirv;
        if (!CompileShader(SPHERES_VERTEX_SHADER_PATH, EShLangVertex, vertex_shader_spirv)) return false;
        SDL_GPUShaderCreateInfo vertex_shader_create_info = {
            .code_size            = vertex_shader_spirv.size() * sizeof(uint32_t),
            .code                 = reinterpret_cast<const uint8_t*>(vertex_shader_spirv.data()),
            .entrypoint           = "main",
            .format               = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage                = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers         = 0,
            .num_storage_textures = 0,
            .num_storage_buffers  = 0,
            .num_uniform_buffers  = 1,
        };
        SDL_GPUShader* vertex_shader = SDL_CreateGPUShader(s_device, &vertex_shader_create_info);
        if (!vertex_shader) {
            SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
            return false;
        }

        // Fragment shader
        std::vector<uint32_t> fragment_shader_spirv;
        if (!CompileShader(SPHERES_FRAGMENT_SHADER_PATH, EShLangFragment, fragment_shader_spirv)) return false;
        SDL_GPUShaderCreateInfo fragment_shader_create_info = {
            .code_size            = fragment_shader_spirv.size() * sizeof(uint32_t),
            .code                 = reinterpret_cast<const uint8_t*>(fragment_shader_spirv.data()),
            .entrypoint           = "main",
            .format               = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage                = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers         = 0,
            .num_storage_textures = 0,
            .num_storage_buffers  = 0,
            .num_uniform_buffers  = 1,
        };
        SDL_GPUShader* fragment_shader = SDL_CreateGPUShader(s_device, &fragment_shader_create_info);
        if (!fragment_shader) {
            SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
            return false;
        }

        // Vertex buffers
        std::array vertex_buffers{
            // Position
            SDL_GPUVertexBufferDescription{
                .slot       = BUFFER_SLOT_POSITIONS,
                .pitch      = sizeof(glm::vec3),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
            },
            // Color
            SDL_GPUVertexBufferDescription{
                .slot       = BUFFER_SLOT_COLORS,
                .pitch      = sizeof(glm::vec3),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
            }
        };

        std::array vertex_attributes{
            SDL_GPUVertexAttribute{ .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 }, // Position
            SDL_GPUVertexAttribute{ .location = 1, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 }, // Color
        };

        std::array color_targets = {
            SDL_GPUColorTargetDescription{
                .format = SDL_GetGPUSwapchainTextureFormat(s_device, s_window)
            }
        };

        SDL_GPUGraphicsPipelineCreateInfo create_info = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffers.data(),
                .num_vertex_buffers         = vertex_buffers.size(),
                .vertex_attributes          = vertex_attributes.data(),
                .num_vertex_attributes      = vertex_attributes.size(),
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .depth_stencil_state = {
                .compare_op          = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test   = true,
                .enable_depth_write  = true,
                .enable_stencil_test = false
            },
            .target_info = {
                .color_target_descriptions = color_targets.data(),
                .num_color_targets        = color_targets.size(),
                .depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                .has_depth_stencil_target = true,
            }
        };
        s_spheres_pipeline = SDL_CreateGPUGraphicsPipeline(s_device, &create_info);
        if (!s_spheres_pipeline) {
            SDL_Log("Failed to create GPU graphics pipeline for spheres: %s", SDL_GetError());
            return false;
        }

        SDL_ReleaseGPUShader(s_device, vertex_shader);
        SDL_ReleaseGPUShader(s_device, fragment_shader);
        return true;
    }

    bool InitializeBoxPipeline() {
        // Vertex shader
        std::vector<uint32_t> vertex_shader_spirv;
        if (!CompileShader(BOX_VERTEX_SHADER_PATH, EShLangVertex, vertex_shader_spirv)) return false;
        SDL_GPUShaderCreateInfo vertex_shader_create_info = {
            .code_size            = vertex_shader_spirv.size() * sizeof(uint32_t),
            .code                 = reinterpret_cast<const uint8_t*>(vertex_shader_spirv.data()),
            .entrypoint           = "main",
            .format               = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage                = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers         = 0,
            .num_storage_textures = 0,
            .num_storage_buffers  = 0,
            .num_uniform_buffers  = 1,
        };
        SDL_GPUShader* vertex_shader = SDL_CreateGPUShader(s_device, &vertex_shader_create_info);
        if (!vertex_shader) {
            SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
            return false;
        }

        // Fragment shader
        std::vector<uint32_t> fragment_shader_spirv;
        if (!CompileShader(BOX_FRAGMENT_SHADER_PATH, EShLangFragment, fragment_shader_spirv)) return false;
        SDL_GPUShaderCreateInfo fragment_shader_create_info = {
            .code_size            = fragment_shader_spirv.size() * sizeof(uint32_t),
            .code                 = reinterpret_cast<const uint8_t*>(fragment_shader_spirv.data()),
            .entrypoint           = "main",
            .format               = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage                = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers         = 0,
            .num_storage_textures = 0,
            .num_storage_buffers  = 0,
            .num_uniform_buffers  = 1,
        };
        SDL_GPUShader* fragment_shader = SDL_CreateGPUShader(s_device, &fragment_shader_create_info);
        if (!fragment_shader) {
            SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
            return false;
        }

        std::array color_targets = {
            SDL_GPUColorTargetDescription{
                .format = SDL_GetGPUSwapchainTextureFormat(s_device, s_window)
            }
        };

        SDL_GPUGraphicsPipelineCreateInfo create_info = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .depth_stencil_state = {
                .compare_op          = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test   = true,
                .enable_depth_write  = true,
                .enable_stencil_test = false
            },
            .target_info = {
                .color_target_descriptions = color_targets.data(),
                .num_color_targets        = color_targets.size(),
                .depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                .has_depth_stencil_target = true,
            }
        };
        s_box_pipeline = SDL_CreateGPUGraphicsPipeline(s_device, &create_info);
        if (!s_spheres_pipeline) {
            SDL_Log("Failed to create GPU graphics pipeline for box: %s", SDL_GetError());
            return false;
        }

        SDL_ReleaseGPUShader(s_device, vertex_shader);
        SDL_ReleaseGPUShader(s_device, fragment_shader);

        return true;
    }
}

bool RendererInitialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    s_window = SDL_CreateWindow("Fluid Simulation", DEFAULT_WND_WIDTH, DEFAULT_WND_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!s_window) {
        printf("Failed to create window: %s", SDL_GetError());
        return false;
    }

    s_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    if (!s_device) {
        printf("Failed to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(s_device, s_window)) {
        printf("Failed to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    s_io = &ImGui::GetIO();
    s_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(s_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = s_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(s_device, s_window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // GLSLang
    glslang::InitializeProcess();

    // Pipelines
    if (!InitializeSpheresPipeline()) {
        return false;
    }
    if (!InitializeBoxPipeline()) {
        return false;
    }

    return true;
}

void RendererShutdown() {
    glslang::FinalizeProcess();

    SDL_ReleaseGPUGraphicsPipeline(s_device, s_box_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(s_device, s_spheres_pipeline);
    SDL_ReleaseGPUTexture(s_device, s_depth_texture);
    SDL_ReleaseGPUBuffer(s_device, s_sphere_positions);
    SDL_ReleaseGPUBuffer(s_device, s_sphere_colors);
    SDL_ReleaseGPUTransferBuffer(s_device, s_transfer_buffer);
    SDL_ReleaseWindowFromGPUDevice(s_device, s_window);

    SDL_WaitForGPUIdle(s_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(s_device, s_window);
    SDL_DestroyGPUDevice(s_device);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}

void RendererSetViewMatrix(const glm::mat4& view) {
    s_view = view;
}

void RendererImGui(std::function<void()> f) {
    s_imgui_func = f;
}

void RendererDrawFrame(
    std::span<const glm::vec3> sphere_positions,
    std::span<const glm::vec3> sphere_colors
) {
    ImDrawData* draw_data = nullptr;

    {
        // ImGui
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        s_imgui_func();

        ImGui::Render();
        draw_data = ImGui::GetDrawData();
    }

    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    // Frame command buffer
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(s_device);

    // Upload sphere data to GPU (CPU-side work; the copy is asynchronous)
    UploadSphereData(command_buffer, sphere_positions, sphere_colors);

    // Swapchain texture; this can block on the previous frame / vsync.
    SDL_GPUTexture* swapchain_texture;
    static uint32_t last_swapchain_texture_width = 0, last_swapchain_texture_height = 0;
    uint32_t swapchain_texture_width, swapchain_texture_height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, s_window, &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height)) {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
    }

    // Resize if needed
    {
        if (last_swapchain_texture_height != swapchain_texture_width || last_swapchain_texture_height != swapchain_texture_height) {
            SDL_GPUTextureCreateInfo depth_texture_create_info = {
                .type                 = SDL_GPU_TEXTURETYPE_2D,
                .format               = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
                .usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
                .width                = swapchain_texture_width,
                .height               = swapchain_texture_height,
                .layer_count_or_depth = 1,
                .num_levels           = 1,
                .sample_count         = SDL_GPU_SAMPLECOUNT_1,
            };
            auto new_depth_texture = SDL_CreateGPUTexture(s_device, &depth_texture_create_info);
            if (!new_depth_texture) {
                SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
            }
            else {
                SDL_ReleaseGPUTexture(s_device, s_depth_texture);
                s_depth_texture = new_depth_texture;
            }
        }
        last_swapchain_texture_width  = swapchain_texture_width;
        last_swapchain_texture_height = swapchain_texture_height;
    }

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

    // Spheres & box render pass
    SDL_GPUColorTargetInfo swapchain_color_target = {
        .texture     = swapchain_texture,
        .clear_color = SDL_FColor{0.2f, 0.4f, 0.6f, 1.0f},
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .store_op    = SDL_GPU_STOREOP_STORE,
    };
    std::array color_target_infos{ swapchain_color_target };

    SDL_GPUDepthStencilTargetInfo depth_target_info = {
        .texture          = s_depth_texture,
        .clear_depth      = 1.0f,
        .load_op          = SDL_GPU_LOADOP_CLEAR,
        .store_op         = SDL_GPU_STOREOP_STORE,
        .stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE
    };

    SDL_GPURenderPass* render_pass = nullptr;

    {

        render_pass = SDL_BeginGPURenderPass(
            command_buffer,
            color_target_infos.data(),
            color_target_infos.size(),
            &depth_target_info
        );

        glm::mat4 projection = glm::perspective(glm::radians(80.0f), (float)swapchain_texture_width / (float)swapchain_texture_height, 0.1f, 100.0f);
        PerFrameUBOSpheres ubo = {
            .view_proj    = projection * s_view,
            .aspect_ratio = (float)swapchain_texture_width / (float)swapchain_texture_height,
            .radius       = g_ui.particle_radius,
        };
        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(PerFrameUBOSpheres));

        // Spheres
        if (s_spheres_num) {
            std::array buffer_bindings{
                SDL_GPUBufferBinding{ .buffer = s_sphere_positions, .offset = 0 },
                SDL_GPUBufferBinding{ .buffer = s_sphere_colors,    .offset = 0 },
            };
            SDL_BindGPUGraphicsPipeline(render_pass, s_spheres_pipeline);
            SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings.data(), buffer_bindings.size());
            SDL_DrawGPUPrimitives(render_pass, 6, s_spheres_num, 0, 0);
        }
        // Box
        SDL_BindGPUGraphicsPipeline(render_pass, s_box_pipeline);
        PerFrameUBOBox box_ubo = {
            .view_proj         = projection * s_view * glm::translate(glm::mat4(1), g_ui.box_size/2.0f) * glm::scale(glm::mat4(1), g_ui.box_size),
            .screen_resolution = glm::vec2(swapchain_texture_width, swapchain_texture_height),
            .line_width        = 4.0f,
        };
        SDL_PushGPUVertexUniformData(command_buffer, 0, &box_ubo, sizeof(box_ubo));
        glm::vec3 box_color(1.0f);
        SDL_PushGPUFragmentUniformData(command_buffer, 0, &box_color, sizeof(box_color));
        SDL_DrawGPUPrimitives(render_pass, 6, 12, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    // ImGui Render pass
    color_target_infos[0].load_op = SDL_GPU_LOADOP_LOAD;
    render_pass = SDL_BeginGPURenderPass(
        command_buffer,
        color_target_infos.data(),
        color_target_infos.size(),
        nullptr
    );
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
    SDL_EndGPURenderPass(render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

