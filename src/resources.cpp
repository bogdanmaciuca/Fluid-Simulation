#include "resources.hpp"

#include "util.hpp"

std::vector<u32> CompileShader(const std::string& path) {
    // Initialize glslang on first call of CompileShader()
    static auto do_once = std::invoke([]() {
        bool success = glslang::InitializeProcess();
        assert(success);
        return 0;
    });

    const auto stage_from_path = [](const std::string& path) {
        if (path.ends_with(".vert")) return EShLangVertex;
        if (path.ends_with(".frag")) return EShLangFragment;
        if (path.ends_with(".comp")) return EShLangCompute;
        assert(false && "Unknown shader extension");
        return EShLangVertex;
    };

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
        return {};
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        std::cerr << "GLSL Linking Failed:\n"
            << program.getInfoLog() << "\n"
            << program.getInfoDebugLog() << std::endl;
        return {};
    }

    std::vector<u32> result;
    glslang::GlslangToSpv(*program.getIntermediate(stage), result);

    return result;
}

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
                if (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) {
                    data.num_readonly_storage_textures += binding.count;
                } else {
                    data.num_readwrite_storage_textures += binding.count;
                }
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                if (binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) {
                    data.num_readonly_storage_buffers += binding.count;
                } else {
                    data.num_readwrite_storage_buffers += binding.count;
                }
                break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                data.num_uniform_buffers += binding.count;
                break;
            default:
                break;
        }
    }

    if (module.entry_point_count > 0) {
        data.threadcount_x = module.entry_points[0].local_size.x;
        data.threadcount_y = module.entry_points[0].local_size.y;
        data.threadcount_z = module.entry_points[0].local_size.z;
    }

    spvReflectDestroyShaderModule(&module);
    return data;
}

GPUDevice CreateGPUDevice() {
    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    assert(device != nullptr);
    return GPUDevice(device);
}

GPUBuffer CreateGPUBuffer(const GPUDevice& device, const SDL_GPUBufferCreateInfo& create_info) {
    auto buffer = SDL_CreateGPUBuffer(device.get(), &create_info);
    assert(buffer != nullptr);
    return GPUBuffer(buffer, GPUResourceDeleter{ device.get() });
}

GPUComputePipeline CreateGPUComputePipeline(const GPUDevice& device, const SDL_GPUComputePipelineCreateInfo& create_info) {
    auto pipeline = SDL_CreateGPUComputePipeline(device.get(), &create_info);
    assert(pipeline != nullptr);
    return GPUComputePipeline(pipeline, GPUResourceDeleter{ device.get() });
}

namespace {
    SDL_GPUComputePipelineCreateInfo MakeComputePipelineCreateInfo(
        const std::vector<u32>& code, const ReflectionData& data) {
        return SDL_GPUComputePipelineCreateInfo{
            .code_size                      = code.size() * sizeof(u32),
            .code                           = reinterpret_cast<const Uint8*>(code.data()),
            .entrypoint                     = "main",
            .format                         = SDL_GPU_SHADERFORMAT_SPIRV,
            .num_samplers                   = data.num_samplers,
            .num_readonly_storage_textures  = data.num_readonly_storage_textures,
            .num_readonly_storage_buffers   = data.num_readonly_storage_buffers,
            .num_readwrite_storage_textures = data.num_readwrite_storage_textures,
            .num_readwrite_storage_buffers  = data.num_readwrite_storage_buffers,
            .num_uniform_buffers            = data.num_uniform_buffers,
            .threadcount_x                  = data.threadcount_x,
            .threadcount_y                  = data.threadcount_y,
            .threadcount_z                  = data.threadcount_z,
        };
    }
}

GPUComputePipeline CreateGPUComputePipeline(const GPUDevice& device, const std::string& path) {
    std::vector<u32> code = CompileShader(path);
    assert(!code.empty());
    ReflectionData data = ReflectShader(code);
    return CreateGPUComputePipeline(device, MakeComputePipelineCreateInfo(code, data));
}

GPUGraphicsPipeline CreateGPUGraphicsPipeline(const GPUDevice& device, const SDL_GPUGraphicsPipelineCreateInfo& create_info) {
    auto pipeline = SDL_CreateGPUGraphicsPipeline(device.get(), &create_info);
    assert(pipeline != nullptr);
    return GPUGraphicsPipeline(pipeline, GPUResourceDeleter{ device.get() });
}

GPUShader CreateGPUShader(const GPUDevice& device, const SDL_GPUShaderCreateInfo& create_info) {
    auto shader = SDL_CreateGPUShader(device.get(), &create_info);
    assert(shader != nullptr);
    return GPUShader(shader, GPUResourceDeleter{ device.get() });
}

GPUShader CreateGPUShader(const GPUDevice& device, const std::string& path) {
    const auto stage_from_path = [](const std::string& path) {
        if (path.ends_with(".vert")) return SDL_GPU_SHADERSTAGE_VERTEX;
        if (path.ends_with(".frag")) return SDL_GPU_SHADERSTAGE_FRAGMENT;
        assert(false && "Unknown shader extension");
        return SDL_GPU_SHADERSTAGE_VERTEX;
    };

    std::vector<u32> code = CompileShader(path);
    assert(!code.empty());
    ReflectionData data = ReflectShader(code);

    SDL_GPUShaderCreateInfo create_info = {
        .code_size            = code.size() * sizeof(u32),
        .code                 = reinterpret_cast<const Uint8*>(code.data()),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = stage_from_path(path),
        .num_samplers         = data.num_samplers,
        .num_storage_textures = data.num_readonly_storage_textures + data.num_readwrite_storage_textures,
        .num_storage_buffers  = data.num_readonly_storage_buffers + data.num_readwrite_storage_buffers,
        .num_uniform_buffers  = data.num_uniform_buffers,
    };
    return CreateGPUShader(device, create_info);
}

GPUTexture CreateGPUTexture(const GPUDevice& device, const SDL_GPUTextureCreateInfo& create_info) {
    auto texture = SDL_CreateGPUTexture(device.get(), &create_info);
    assert(texture != nullptr);
    return GPUTexture(texture, GPUResourceDeleter{ device.get() });
}

GPUSampler CreateGPUSampler(const GPUDevice& device, const SDL_GPUSamplerCreateInfo& create_info) {
    auto sampler = SDL_CreateGPUSampler(device.get(), &create_info);
    assert(sampler != nullptr);
    return GPUSampler(sampler, GPUResourceDeleter{ device.get() });
}

GPUComputePipelineEx::GPUComputePipelineEx(const GPUDevice& device, const std::string& path)
    : m_device(device.get())
    , m_path(path)
    , m_last_write_time(std::filesystem::last_write_time(path)) {
    RebuildPipeline();
}

bool GPUComputePipelineEx::RebuildPipeline() {
    std::vector<u32> code = CompileShader(m_path);
    if (code.empty()) {
        std::cerr << "Failed to compile compute shader: " << m_path << std::endl;
        return false;
    }

    ReflectionData data = ReflectShader(code);

    SDL_GPUComputePipelineCreateInfo create_info = MakeComputePipelineCreateInfo(code, data);
    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(m_device, &create_info);
    if (pipeline == nullptr) {
        std::cerr << "Failed to create compute pipeline from shader: " << m_path << std::endl;
        return false;
    }

    m_thread_count_x = data.threadcount_x;
    m_thread_count_y = data.threadcount_y;
    m_thread_count_z = data.threadcount_z;

    m_pipeline = GPUComputePipeline(pipeline, GPUResourceDeleter{ m_device });
    return true;
}

void GPUComputePipelineEx::ReloadIfModified() {
    auto current = std::filesystem::last_write_time(m_path);
    if (current == m_last_write_time) {
        return;
    }

    if (RebuildPipeline()) {
        m_last_write_time = current;
    }
}

void GPUComputePipelineEx::SetBindings(
    std::span<const SDL_GPUStorageTextureReadWriteBinding> textures,
    std::span<const SDL_GPUStorageBufferReadWriteBinding> buffers) {
    m_texture_bindings.assign(textures.begin(), textures.end());
    m_buffer_bindings.assign(buffers.begin(), buffers.end());
}

void GPUComputePipelineEx::DispatchAsPass(
    SDL_GPUCommandBuffer* command_buffer, u32 work_x, u32 work_y, u32 work_z) const {
    SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        m_texture_bindings.data(), static_cast<Uint32>(m_texture_bindings.size()),
        m_buffer_bindings.data(), static_cast<Uint32>(m_buffer_bindings.size()));
    SDL_BindGPUComputePipeline(compute_pass, m_pipeline.get());

    u32 groupcount_x = (work_x + m_thread_count_x - 1) / m_thread_count_x;
    u32 groupcount_y = (work_y + m_thread_count_y - 1) / m_thread_count_y;
    u32 groupcount_z = (work_z + m_thread_count_z - 1) / m_thread_count_z;
    SDL_DispatchGPUCompute(compute_pass, groupcount_x, groupcount_y, groupcount_z);

    SDL_EndGPUComputePass(compute_pass);
}
