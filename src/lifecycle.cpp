#include "lifecycle.hpp"

#include "globals.hpp"
#include "helpers.hpp"

constexpr u32 DEFAULT_WND_W = 1200;
constexpr u32 DEFAULT_WND_H = 800;

namespace {
    void InitializeSSBO() {
        SDL_GPUBufferUsageFlags usage = 
            SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ |
            SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ |
            SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        SDL_GPUBufferCreateInfo create_info = { .usage = usage, .size = SSBO_SIZE };

        globals.ssbo = SDL_CreateGPUBuffer(globals.device, &create_info);
        assert(globals.ssbo != nullptr);
    }

    void InitialiazeCirclesPipeline() {
        auto& g = globals;

        SDL_GPUShader* vs = CreateShader(FLUID_VS_PATH);
        SDL_GPUShader* fs = CreateShader(FLUID_FS_PATH);

        std::array color_targets = {
            SDL_GPUColorTargetDescription{
                .format = SDL_GetGPUSwapchainTextureFormat(g.device, g.window)
            }
        };

        SDL_GPUGraphicsPipelineCreateInfo create_info = {
            .vertex_shader = vs,
            .fragment_shader = fs,
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
        auto new_circles_pipeline = SDL_CreateGPUGraphicsPipeline(g.device, &create_info);
        if (new_circles_pipeline == nullptr) {
            std::cerr << "Error: could not create new circles pipeline\n";
        }

        if (g.circles_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(g.device, g.circles_pipeline);
        }

        SDL_ReleaseGPUShader(g.device, vs);
        SDL_ReleaseGPUShader(g.device, fs);
    }

    void InitializeBoxPipeline() {
        assert(false && "Not implemented yet");
    }

    void InitializeSDL() {
        auto& g = globals;

        SDL_Init(SDL_INIT_VIDEO);

        g.window = SDL_CreateWindow("Naval Simulator", DEFAULT_WND_W, DEFAULT_WND_H, SDL_WINDOW_RESIZABLE);
        assert(g.window != nullptr);

        g.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
        assert(g.device != nullptr);
        SDL_ClaimWindowForGPUDevice(g.device, g.window);
    }

    void ShutdownSDL() {
        auto& g = globals;

        SDL_WaitForGPUIdle(g.device);
        SDL_DestroyGPUDevice(g.device);
        SDL_DestroyWindow(g.window);
        SDL_Quit();
    }

    void InitializeImGui() {
        auto& g = globals;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        g.io = &ImGui::GetIO();

        ImGui_ImplSDL3_InitForSDLGPU(g.window);
        ImGui_ImplSDLGPU3_InitInfo init_info = {
            .Device               = g.device,
            .ColorTargetFormat    = SDL_GetGPUSwapchainTextureFormat(g.device, g.window),
            .MSAASamples          = SDL_GPU_SAMPLECOUNT_1,
            .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            .PresentMode          = SDL_GPU_PRESENTMODE_VSYNC,
        };
        ImGui_ImplSDLGPU3_Init(&init_info);
    }

    void ShutdownImGui() {
        SDL_WaitForGPUIdle(globals.device);
        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();
    }
}

void Initialize() {
    InitializeSDL();
    InitializeImGui();
    // InitialiazeCirclesPipeline();
    // InitializeBoxPipeline();
}

void Shutdown() {
    auto& g = globals;

    SDL_ReleaseGPUGraphicsPipeline(g.device, g.box_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g.device, g.circles_pipeline);
    SDL_ReleaseGPUBuffer(g.device, g.ssbo);
    ShutdownImGui();
    ShutdownSDL();
}


