#include "helpers.hpp"

#include "imgui.h"
#include "log.hpp"
#include "util.hpp"

namespace {
    u32 VertexElementFormatSize(SDL_GPUVertexElementFormat format) {
        switch (format) {
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:  return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2: return 8;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3: return 12;
            case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4: return 16;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT:    return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT2:   return 8;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT3:   return 12;
            case SDL_GPU_VERTEXELEMENTFORMAT_INT4:   return 16;
            case SDL_GPU_VERTEXELEMENTFORMAT_UINT:   return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_UINT2:  return 8;
            case SDL_GPU_VERTEXELEMENTFORMAT_UINT3:  return 12;
            case SDL_GPU_VERTEXELEMENTFORMAT_UINT4:  return 16;
            case SDL_GPU_VERTEXELEMENTFORMAT_HALF2:  return 4;
            case SDL_GPU_VERTEXELEMENTFORMAT_HALF4:  return 8;
            default: return 0;
        }
    }
}

std::optional<std::vector<u32>> CompileShader(const std::string& path, EShLanguage stage, const ShaderDefineVector& defines) {
    // Initialize glslang on first call of CompileShader()
    static const bool glslang_initialized = glslang::InitializeProcess();
    if (!glslang_initialized) {
        LogError("Failed to initialize glslang");
        return std::nullopt;
    }

    std::optional<std::string> source = ReadFile(path);
    if (!source) {
        LogError("Failed to read shader file: {}", path);
        return std::nullopt;
    }
    const char* c_source = source->c_str();

    glslang::TShader shader(stage);
    shader.setStrings(&c_source, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 450);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    std::string preamble;
    for (const auto& [name, value] : defines) {
        preamble += "#define " + name;
        if (!value.empty()) {
            preamble += " " + value;
        }
        preamble += "\n";
    }
    shader.setPreamble(preamble.c_str());

    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        std::string stage_str;
        switch (stage) {
            case EShLangVertex: stage_str = "Vertex"; break;
            case EShLangFragment: stage_str = "Fragment"; break;
            case EShLangCompute: stage_str = "Compute"; break;
            default: stage_str = "Unknown"; break;
        }
        LogError("GLSL Compilation Failed ({}):\n{}\n{}", stage_str, shader.getInfoLog(), shader.getInfoDebugLog());
        return std::nullopt;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        LogError("GLSL Linking Failed:\n{}\n{}", program.getInfoLog(), program.getInfoDebugLog());
        return std::nullopt;
    }

    std::vector<u32> result;
    glslang::GlslangToSpv(*program.getIntermediate(stage), result);

    return result;
}

std::optional<ReflectionData> ReflectShader(std::span<u32> spirv_code) {
    if (spirv_code.empty()) {
        LogError("Cannot reflect! Empty SPIRV code vector");
        return std::nullopt;
    }

    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(
        spirv_code.size_bytes(), spirv_code.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        LogError("Reflection failed! SpvReflect error {}", static_cast<int>(result));
        return std::nullopt;
    }

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

    const auto ToVertexElementFormat = [](SpvReflectFormat format) {
        switch (format) {
            case SPV_REFLECT_FORMAT_R32_SFLOAT:          return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
            case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;

            case SPV_REFLECT_FORMAT_R32_SINT:            return SDL_GPU_VERTEXELEMENTFORMAT_INT;
            case SPV_REFLECT_FORMAT_R32G32_SINT:         return SDL_GPU_VERTEXELEMENTFORMAT_INT2;
            case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return SDL_GPU_VERTEXELEMENTFORMAT_INT3;
            case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return SDL_GPU_VERTEXELEMENTFORMAT_INT4;

            case SPV_REFLECT_FORMAT_R32_UINT:            return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
            case SPV_REFLECT_FORMAT_R32G32_UINT:         return SDL_GPU_VERTEXELEMENTFORMAT_UINT2;
            case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return SDL_GPU_VERTEXELEMENTFORMAT_UINT3;
            case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return SDL_GPU_VERTEXELEMENTFORMAT_UINT4;

            case SPV_REFLECT_FORMAT_R16G16_SFLOAT:       return SDL_GPU_VERTEXELEMENTFORMAT_HALF2;
            case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_HALF4;

            default:                                     return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
        }
    };

    struct ReflectedAttribute {
        u32 location;
        SDL_GPUVertexElementFormat format;
    };
    std::vector<ReflectedAttribute> attributes;
    bool unsupported_format = false;
    if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
        for (u32 i = 0; i < module.input_variable_count; ++i) {
            const SpvReflectInterfaceVariable& variable = *module.input_variables[i];
            if (variable.built_in >= 0) {
                continue;
            }
            if (variable.location == 0xFFFFFFFF) {
                continue;
            }

            SDL_GPUVertexElementFormat format = ToVertexElementFormat(variable.format);
            if (format == SDL_GPU_VERTEXELEMENTFORMAT_INVALID) {
                LogError("Unsupported vertex input format at location {}", variable.location);
                unsupported_format = true;
                break;
            }

            attributes.push_back({ variable.location, format });
        }
    }

    if (unsupported_format) {
        spvReflectDestroyShaderModule(&module);
        return std::nullopt;
    }

    std::sort(attributes.begin(), attributes.end(), [](const ReflectedAttribute& a, const ReflectedAttribute& b) {
        return a.location < b.location;
    });

    u32 offset = 0;
    for (const ReflectedAttribute& attribute : attributes) {
        u32 size = VertexElementFormatSize(attribute.format);
        if (size == 0) {
            LogError("No vertex element size for format at location {}", attribute.location);
            spvReflectDestroyShaderModule(&module);
            return std::nullopt;
        }

        data.vertex_attributes.push_back({
            .location    = attribute.location,
            .buffer_slot = 0,
            .format      = attribute.format,
            .offset      = offset,
        });
        offset += size;
    }

    spvReflectDestroyShaderModule(&module);
    return data;
}

std::optional<Window> CreateWindow(const std::string& title, int w, int h) {
    auto window = SDL_CreateWindow(title.c_str(), w, h, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        LogError("Failed to create window: {}", SDL_GetError());
        return std::nullopt;
    }
    return Window(window);
}

std::optional<GPUDevice> CreateGPUDevice() {
    auto device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    if (device == nullptr) {
        LogError("Failed to create GPU device: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUDevice(device);
}

std::optional<GPUBuffer> CreateGPUBuffer(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo& create_info) {
    auto buffer = SDL_CreateGPUBuffer(device, &create_info);
    if (buffer == nullptr) {
        LogError("Failed to create GPU buffer: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUBuffer(buffer, ResourceDeleter{ device });
}

std::optional<GPUComputePipeline> CreateGPUComputePipeline(SDL_GPUDevice* device, const SDL_GPUComputePipelineCreateInfo& create_info) {
    auto pipeline = SDL_CreateGPUComputePipeline(device, &create_info);
    if (pipeline == nullptr) {
        LogError("Failed to create GPU compute pipeline: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUComputePipeline(pipeline, ResourceDeleter{ device });
}

std::optional<GPUComputePipeline> CreateGPUComputePipeline(
    SDL_GPUDevice* device,
    const std::string& path,
    ShaderDefineVector defines,
    ReflectionData* reflection
) {
    std::optional<std::vector<u32>> code = CompileShader(path, EShLangCompute, defines);
    if (!code) {
        return std::nullopt;
    }

    std::optional<ReflectionData> data = ReflectShader(*code);
    if (!data) {
        return std::nullopt;
    }

    std::optional<GPUComputePipeline> pipeline = CreateGPUComputePipeline(device, {
        .code_size                      = code->size() * sizeof(u32),
        .code                           = reinterpret_cast<const Uint8*>(code->data()),
        .entrypoint                     = "main",
        .format                         = SDL_GPU_SHADERFORMAT_SPIRV,
        .num_samplers                   = data->num_samplers,
        .num_readonly_storage_textures  = data->num_readonly_storage_textures,
        .num_readonly_storage_buffers   = data->num_readonly_storage_buffers,
        .num_readwrite_storage_textures = data->num_readwrite_storage_textures,
        .num_readwrite_storage_buffers  = data->num_readwrite_storage_buffers,
        .num_uniform_buffers            = data->num_uniform_buffers,
        .threadcount_x                  = data->threadcount_x,
        .threadcount_y                  = data->threadcount_y,
        .threadcount_z                  = data->threadcount_z,
    });
    if (!pipeline) {
        return std::nullopt;
    }

    if (reflection) {
        *reflection = *data;
    }

    return pipeline;
}

std::optional<GPUGraphicsPipeline> CreateGPUGraphicsPipeline(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& create_info) {
    auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
    if (pipeline == nullptr) {
        LogError("Failed to create GPU graphics pipeline: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUGraphicsPipeline(pipeline, ResourceDeleter{ device });
}

std::optional<GPUShader> CreateGPUShader(SDL_GPUDevice* device, const SDL_GPUShaderCreateInfo& create_info, const ShaderDefineVector& defines) {
    auto shader = SDL_CreateGPUShader(device, &create_info);
    if (shader == nullptr) {
        LogError("Failed to create GPU shader: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUShader(shader, ResourceDeleter{ device });
}

std::optional<GPUShader> CreateGPUShader(
    SDL_GPUDevice* device,
    const std::string& path,
    const ShaderDefineVector& defines,
    ReflectionData* reflection
) {
    SDL_GPUShaderStage sdl_stage;
    EShLanguage glslang_stage;

    if (path.ends_with(".vert")) {
        return CreateGPUShaderVertex(device, path, reflection);
    }
    else if (path.ends_with(".frag")) {
        return CreateGPUShaderFragment(device, path, reflection);
    }
    else {
        LogError("Cannot deduce shader type from this extension: {}", path);
        return std::nullopt;
    }
}

std::optional<GPUShader> CreateGPUShaderVertex(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection) {
    std::optional<std::vector<u32>> code = CompileShader(path, EShLangVertex, {{"VS", ""}});
    if (!code) {
        return std::nullopt;
    }

    std::optional<ReflectionData> data = ReflectShader(*code);
    if (!data) {
        return std::nullopt;
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size            = code->size() * sizeof(u32),
        .code                 = reinterpret_cast<const Uint8*>(code->data()),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers         = data->num_samplers,
        .num_storage_textures = data->num_readonly_storage_textures + data->num_readwrite_storage_textures,
        .num_storage_buffers  = data->num_readonly_storage_buffers + data->num_readwrite_storage_buffers,
        .num_uniform_buffers  = data->num_uniform_buffers,
    };

    std::optional<GPUShader> shader = CreateGPUShader(device, create_info);
    if (!shader) {
        return std::nullopt;
    }

    if (reflection) {
        *reflection = *data;
    }

    return shader;
}

std::optional<GPUShader> CreateGPUShaderFragment(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection) {
    std::optional<std::vector<u32>> code = CompileShader(path, EShLangFragment, {{"FS", ""}});
    if (!code) {
        return std::nullopt;
    }

    std::optional<ReflectionData> data = ReflectShader(*code);
    if (!data) {
        return std::nullopt;
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size            = code->size() * sizeof(u32),
        .code                 = reinterpret_cast<const Uint8*>(code->data()),
        .entrypoint           = "main",
        .format               = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage                = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers         = data->num_samplers,
        .num_storage_textures = data->num_readonly_storage_textures + data->num_readwrite_storage_textures,
        .num_storage_buffers  = data->num_readonly_storage_buffers + data->num_readwrite_storage_buffers,
        .num_uniform_buffers  = data->num_uniform_buffers,
    };

    std::optional<GPUShader> shader = CreateGPUShader(device, create_info);
    if (!shader) {
        return std::nullopt;
    }

    if (reflection) {
        *reflection = *data;
    }

    return shader;
}

std::optional<GPUTexture> CreateGPUTexture(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info) {
    auto texture = SDL_CreateGPUTexture(device, &create_info);
    if (texture == nullptr) {
        LogError("Failed to create GPU texture: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUTexture(texture, ResourceDeleter{ device });
}

std::optional<GPUTransferBuffer> CreateGPUTransferBuffer(SDL_GPUDevice* device, const SDL_GPUTransferBufferCreateInfo& create_info) {
    auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &create_info);
    if (transfer_buffer == nullptr) {
        LogError("Failed to create GPU transfer buffer: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUTransferBuffer(transfer_buffer, ResourceDeleter{ device });
}

std::optional<GPUSampler> CreateGPUSampler(SDL_GPUDevice* device, const SDL_GPUSamplerCreateInfo& create_info) {
    auto sampler = SDL_CreateGPUSampler(device, &create_info);
    if (sampler == nullptr) {
        LogError("Failed to create GPU sampler: {}", SDL_GetError());
        return std::nullopt;
    }
    return GPUSampler(sampler, ResourceDeleter{ device });
}

std::optional<GPUComputePipelineEx> GPUComputePipelineEx::Create(
    SDL_GPUDevice* device,
    const std::string& path,
    const ShaderDefineVector& defines
) {
    GPUComputePipelineEx pipeline;
    pipeline.m_path = path;
    pipeline.SetDevice(device);

    std::error_code ec;
    pipeline.m_last_write_time = std::filesystem::last_write_time(path, ec);
    if (ec) {
        LogError("Failed to stat shader file {}: {}", path, ec.message());
        return std::nullopt;
    }

    pipeline.m_defines = defines;
    if (!pipeline.RebuildPipeline()) {
        return std::nullopt;
    }
    return pipeline;
}

bool GPUComputePipelineEx::ReloadIfModified() {
    std::error_code ec;
    auto current = std::filesystem::last_write_time(m_path, ec);
    if (ec) {
        return false;
    }
    if (current == m_last_write_time) {
        return true;
    }

    m_last_write_time = current;
    return RebuildPipeline();
}

void GPUComputePipelineEx::SetBindings(
    std::span<const SDL_GPUStorageTextureReadWriteBinding> textures,
    std::span<const SDL_GPUStorageBufferReadWriteBinding> buffers) {
    m_texture_bindings.assign(textures.begin(), textures.end());
    m_buffer_bindings.assign(buffers.begin(), buffers.end());
}

bool GPUComputePipelineEx::SetDefines(ShaderDefineVector defines, bool rebuild) {
    m_defines = defines;
    if (rebuild) {
        return RebuildPipeline();
    }
    return true;
}

void GPUComputePipelineEx::DispatchAsPass(
    SDL_GPUCommandBuffer* command_buffer,
    u32 work_x, u32 work_y, u32 work_z
) const {
    if (!IsValid()) {
        LogError("Cannot dispatch compute pass: pipeline is null");
        return;
    }

    SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(
        command_buffer,
        m_texture_bindings.data(), static_cast<Uint32>(m_texture_bindings.size()),
        m_buffer_bindings.data(), static_cast<Uint32>(m_buffer_bindings.size())
    );
    SDL_BindGPUComputePipeline(compute_pass, Get());

    u32 groupcount_x = (work_x + m_thread_count_x - 1) / m_thread_count_x;
    u32 groupcount_y = (work_y + m_thread_count_y - 1) / m_thread_count_y;
    u32 groupcount_z = (work_z + m_thread_count_z - 1) / m_thread_count_z;
    SDL_DispatchGPUCompute(compute_pass, groupcount_x, groupcount_y, groupcount_z);

    SDL_EndGPUComputePass(compute_pass);
}

bool GPUComputePipelineEx::RebuildPipeline() {
    ReflectionData reflection{};
    std::optional<GPUComputePipeline> new_pipeline = CreateGPUComputePipeline(
        Device(), m_path, m_defines, &reflection);

    if (!new_pipeline) {
        return false;
    }

    Set(std::move(*new_pipeline));

    m_thread_count_x = reflection.threadcount_x;
    m_thread_count_y = reflection.threadcount_y;
    m_thread_count_z = reflection.threadcount_z;

    return true;
}

std::optional<GPUGraphicsPipelineEx> GPUGraphicsPipelineEx::Create(
    SDL_GPUDevice* device,
    const std::string& path,
    const SDL_GPUGraphicsPipelineCreateInfo& create_info
) {
    GPUGraphicsPipelineEx pipeline;
    pipeline.m_path = path;

    std::error_code ec;
    pipeline.m_last_write_time = std::filesystem::last_write_time(path, ec);
    if (ec) {
        LogError("Failed to stat shader file {}: {}", path, ec.message());
        return std::nullopt;
    }

    pipeline.m_create_info = create_info;

    pipeline.SetDevice(device);

    if (create_info.target_info.num_color_targets > 0 && create_info.target_info.color_target_descriptions != nullptr) {
        pipeline.m_color_targets.assign(
            create_info.target_info.color_target_descriptions,
            create_info.target_info.color_target_descriptions + create_info.target_info.num_color_targets
        );
    }
    pipeline.m_create_info.target_info.color_target_descriptions =
        pipeline.m_color_targets.empty() ? nullptr : pipeline.m_color_targets.data();

    if (!pipeline.RebuildPipeline()) {
        return std::nullopt;
    }
    return pipeline;
}

bool GPUGraphicsPipelineEx::RebuildPipeline() {
    ReflectionData vs_refl{};
    std::optional<GPUShader> vs = CreateGPUShaderVertex(Device(), m_path, &vs_refl);
    if (!vs) {
        return false;
    }

    std::optional<GPUShader> fs = CreateGPUShaderFragment(Device(), m_path);
    if (!fs) {
        return false;
    }

    u32 pitch = 0;
    for (const SDL_GPUVertexAttribute& attribute : vs_refl.vertex_attributes) {
        pitch = std::max(pitch, attribute.offset + VertexElementFormatSize(attribute.format));
    }

    bool has_vertex_attributes = !vs_refl.vertex_attributes.empty();

    m_vertex_buffers.clear();
    if (has_vertex_attributes) {
        m_vertex_buffers.push_back({
            .slot               = 0,
            .pitch              = pitch,
            .input_rate         = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
        });
    }

    m_create_info.target_info.color_target_descriptions =
        m_color_targets.empty() ? nullptr : m_color_targets.data();

    m_create_info.vertex_shader = vs->get();
    m_create_info.fragment_shader = fs->get();
    m_create_info.vertex_input_state = {
        .vertex_buffer_descriptions = m_vertex_buffers.empty() ? nullptr : m_vertex_buffers.data(),
        .num_vertex_buffers         = static_cast<Uint32>(m_vertex_buffers.size()),
        .vertex_attributes          = has_vertex_attributes ? vs_refl.vertex_attributes.data() : nullptr,
        .num_vertex_attributes      = has_vertex_attributes ? static_cast<Uint32>(vs_refl.vertex_attributes.size()) : 0u,
    };

    std::optional<GPUGraphicsPipeline> new_pipeline = CreateGPUGraphicsPipeline(
        Device(), m_create_info);
    if (!new_pipeline) {
        m_create_info.vertex_shader = nullptr;
        m_create_info.fragment_shader = nullptr;
        m_create_info.vertex_input_state.vertex_attributes = nullptr;
        return false;
    }

    m_create_info.vertex_shader = nullptr;
    m_create_info.fragment_shader = nullptr;
    m_create_info.vertex_input_state.vertex_attributes = nullptr;
    Set(std::move(*new_pipeline));

    return true;
}

bool GPUGraphicsPipelineEx::ReloadIfModified() {
    std::error_code ec;
    auto current = std::filesystem::last_write_time(m_path, ec);
    if (ec) {
        return false;
    }
    if (current == m_last_write_time) {
        return true;
    }

    m_last_write_time = current;
    return RebuildPipeline();
}

std::optional<GPUTextureEx> GPUTextureEx::Create(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info) {
    GPUTextureEx texture_ex;
    std::optional<GPUTexture> texture = CreateGPUTexture(device, create_info);
    if (!texture) {
        return std::nullopt;
    }

    texture_ex.SetDevice(device);
    texture_ex.Set(std::move(*texture));
    texture_ex.m_create_info = create_info;
    return texture_ex;
}

bool GPUTextureEx::Recreate(u32 width, u32 height, u32 depth) {
    if (width == 0 || height == 0 || depth == 0) {
        LogError("Cannot create texture with dimensions {}x{}x{}", width, height, depth);
        return false;
    }
    if (Device() == nullptr) {
        LogError("Cannot recreate texture without a GPU device");
        return false;
    }

    SDL_GPUTextureCreateInfo create_info = m_create_info;
    create_info.width                = width;
    create_info.height               = height;
    create_info.layer_count_or_depth = depth;

    std::optional<GPUTexture> new_texture = CreateGPUTexture(Device(), create_info);
    if (!new_texture) {
        return false;
    }

    Set(std::move(*new_texture));
    m_create_info = create_info;
    return true;
}

bool GPUTextureEx::EnsureSize(u32 width, u32 height, u32 depth) {
    if (
        IsValid() &&
        m_create_info.width >= width &&
        m_create_info.height >= height &&
        m_create_info.layer_count_or_depth >= depth
    ) {
        return true;
    }

    return Recreate(
        std::max(m_create_info.width, width),
        std::max(m_create_info.height, height),
        std::max(m_create_info.layer_count_or_depth, depth)
    );
}

bool GPUTextureEx::Resize(u32 width, u32 height, u32 depth) {
    if (
        IsValid() &&
        m_create_info.width == width &&
        m_create_info.height == height &&
        m_create_info.layer_count_or_depth == depth
    ) {
        return true;
    }

    return Recreate(width, height, depth);
}

ImGuiHelper::ImGuiHelper(SDL_GPUDevice* device, SDL_Window* window) :
    m_device(device)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window)
    };
    ImGui_ImplSDLGPU3_Init(&init_info);
    m_io = &ImGui::GetIO();
    m_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

ImGuiHelper::~ImGuiHelper() {
    SDL_WaitForGPUIdle(m_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiHelper::NewFrame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiHelper::EndFrame() {
    ImGui::Render();
}

void ImGuiHelper::DispatchRenderPass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* texture, bool clear) {
    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);

    SDL_GPUColorTargetInfo color_target = {
        .texture     = texture,
        .load_op     = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
        .store_op    = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
        command_buffer, &color_target, 1, nullptr);
    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);
    SDL_EndGPURenderPass(render_pass);
}

void ImGuiHelper::ControlPanelAndScene(GPUTextureEx& scene_texture, std::function<void()> control_panel, bool* scene_is_focused) {
    NewFrame();

    ImGui::DockSpaceOverViewport(
        0,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode
    );

    ImGui::Begin("Control Panel", nullptr, 0);
    control_panel();
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Simulation", nullptr, 0);

    if (scene_is_focused) {
        *scene_is_focused = ImGui::IsWindowHovered();
    }

    auto available_scene_region = ImGui::GetContentRegionAvail();
    scene_texture.Resize(available_scene_region.x, available_scene_region.y);
    ImGui::Image(
        (ImTextureID)scene_texture.Get(),
        ImVec2(scene_texture.GetCreateInfo().width, scene_texture.GetCreateInfo().height)
    );

    ImGui::End();

    ImGui::PopStyleVar(2);
    EndFrame();
}

