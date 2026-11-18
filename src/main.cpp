// TODO:
// - initialize SSBO

#include "camera.hpp"
#include "globals.hpp"
#include "main_loop.hpp"

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());

    std::ostringstream ss;
    ss << file.rdbuf();
    assert(!file.bad());

    return ss.str();
}

bool CompileShader(const std::string& path, std::vector<uint32_t>& result) {
    // Initialize glslang on first call of CompileShader()
    static auto do_once = []() {
        bool success = glslang::InitializeProcess();
        assert(success);
        return 0;
    }();


    const auto stage_from_path = [](const std::string& path) {
        if (path.ends_with(".vert")) return EShLangVertex;
        if (path.ends_with(".frag")) return EShLangFragment;
        if (path.ends_with(".comp")) return EShLangCompute;
        assert(false && "Unknown shader extension");
        return EShLangVertex;
    };

    result.clear();

    const EShLanguage stage = stage_from_path(path);
    std::string source = ReadFile(path);
    const char* c_source = source.c_str();

    glslang::TShader shader(stage);
    shader.setStrings(&c_source, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        std::string stage_str;
        switch (stage) {
            case EShLangVertex: stage_str = "Vertex"; break;
            case EShLangFragment: stage_str = "Fragment"; break;
            case EShLangCompute: stage_str = "Compute"; break;
            default: assert(false);
        }
        std::cerr << "GLSL Compilation Failed (" << stage_str << "):\n"
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

    return true;
}

struct ReflectionData {
    u32 num_samplers;
    u32 num_storage_textures;
    u32 num_storage_buffers;
    u32 num_uniform_buffers;
};

ReflectionData ReflectShader(std::span<u32> spirv_code) {
    assert(!spirv_code.empty());

    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(
        spirv_code.size_bytes(), spirv_code.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    ReflectionData data = {};
    for (u32 i = 0; i < module.descriptor_binding_count; ++i) {
        const SpvReflectDescriptorBinding& binding = module.descriptor_bindings[i];
        switch (binding.descriptor_type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                data.num_samplers += binding.count;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                data.num_storage_textures += binding.count;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                data.num_storage_buffers += binding.count;
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                data.num_uniform_buffers += binding.count;
                break;
            default:
                break;
        }
    }

    spvReflectDestroyShaderModule(&module);
    return data;
};

SDL_GPUShader* CreateShader(const std::string& path) {
    const auto sdl_stage_from_path = [](const std::string& path) {
        if (path.ends_with(".vert")) return SDL_GPU_SHADERSTAGE_VERTEX;
        if (path.ends_with(".frag")) return SDL_GPU_SHADERSTAGE_FRAGMENT;
        assert(false && "Compute shaders are created via compute pipelines");
        return SDL_GPU_SHADERSTAGE_VERTEX;
    };

    std::vector<u32> spv;
    bool compiled = CompileShader(path, spv);
    assert(compiled);

    const ReflectionData reflection = ReflectShader(spv);

    SDL_GPUShaderCreateInfo create_info = {
        .code_size            = spv.size() * sizeof(u32),
        .code                 = reinterpret_cast<const Uint8*>(spv.data()),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = sdl_stage_from_path(path),
        .num_samplers         = reflection.num_samplers,
        .num_storage_textures = reflection.num_storage_textures,
        .num_storage_buffers  = reflection.num_storage_buffers,
        .num_uniform_buffers  = reflection.num_uniform_buffers,
    };

    SDL_GPUShader* shader = SDL_CreateGPUShader(globals.device, &create_info);
    assert(shader != nullptr);
    return shader;
}

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
    g.circles_pipeline = SDL_CreateGPUGraphicsPipeline(g.device, &create_info);
    assert(g.circles_pipeline != nullptr);
}

void InitializeBoxPipeline() {
    assert(false && "Not implemented yet");
}

void InitializeSDL() {
    auto& g = globals;

    SDL_Init(SDL_INIT_VIDEO);

    g.window = SDL_CreateWindow("Naval Simulator", 1200, 800, SDL_WINDOW_RESIZABLE); assert(g.window != nullptr);

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

void Initialize() {
    InitializeSDL();
    InitialiazeCirclesPipeline();
    InitializeBoxPipeline();
}

void Shutdown() {
    auto& g = globals;

    SDL_ReleaseGPUGraphicsPipeline(g.device, g.box_pipeline);
    SDL_ReleaseGPUGraphicsPipeline(g.device, g.circles_pipeline);
    SDL_ReleaseGPUBuffer(g.device, g.ssbo);
    ShutdownSDL();
}

int main() {

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

    Camera camera;

    MainLoop({
        .variable_update = [&](float delta_time) {
            // Process mouse movement for camera only if LMB is down
            float dx, dy;
            SDL_MouseButtonFlags mouse_flags = SDL_GetRelativeMouseState(&dx, &dy);
            if (mouse_flags & SDL_BUTTON_LMASK) {
                camera.ProcessMouseMovement(dx, -dy);
            }

            // Process keyboard
            if (globals.keys[SDL_SCANCODE_W].down) {
                camera.ProcessKeyboard(CameraMovement::Forward, delta_time);
            }
            else if (globals.keys[SDL_SCANCODE_S].down) {
                camera.ProcessKeyboard(CameraMovement::Backward, delta_time);
            }
            if (globals.keys[SDL_SCANCODE_A].down) {
                camera.ProcessKeyboard(CameraMovement::Left, delta_time);
            }
            else if (globals.keys[SDL_SCANCODE_D].down) {
                camera.ProcessKeyboard(CameraMovement::Right, delta_time);
            }

            if (globals.keys[SDL_SCANCODE_R].pressed) {
                std::cout << "Pressed R!\n";
            }
        },

        .fixed_update = [&](float timestep) {
            // std::cout << camera.GetZoom() << "\n";
        },

        .on_event = [&](const SDL_Event& event) -> bool {
            switch (event.type) {
                case SDL_EVENT_MOUSE_WHEEL:
                    camera.ProcessMouseScroll(event.wheel.y);
                    return true;
            }
            return false;
        }
    });

    return 0;
}

