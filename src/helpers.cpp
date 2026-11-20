#include "helpers.hpp"

#include "globals.hpp"
#include "util.hpp"

namespace {
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
            assert(false && "GLSL compilation failed");
        }

        glslang::TProgram program;
        program.addShader(&shader);

        if (!program.link(messages)) {
            std::cerr << "GLSL Linking Failed:\n"
                << program.getInfoLog() << "\n"
                << program.getInfoDebugLog() << std::endl;
            assert("GLSL linking failed");
        }

        std::vector<u32> result;
        glslang::GlslangToSpv(*program.getIntermediate(stage), result);

        return result;
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


}

SDL_GPUShader* CreateShader(const std::string& path) {
    const auto sdl_stage_from_path = [](const std::string& path) {
        if (path.ends_with(".vert")) return SDL_GPU_SHADERSTAGE_VERTEX;
        if (path.ends_with(".frag")) return SDL_GPU_SHADERSTAGE_FRAGMENT;
        assert(false && "Compute shaders are created via compute pipelines");
        return SDL_GPU_SHADERSTAGE_VERTEX;
    };

    std::vector<u32> spv = CompileShader(path);

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

SDL_GPUTexture* CreateDepthTexture(u32 w, u32 h) {
    auto& g = globals;

    SDL_GPUTextureCreateInfo create_info = {
        .type                 = SDL_GPU_TEXTURETYPE_2D,
        .format               = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width                = w,
        .height               = h,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
        .sample_count         = SDL_GPU_SAMPLECOUNT_1,
    };
    SDL_GPUTexture* depth_texture = SDL_CreateGPUTexture(g.device, &create_info);
    assert(depth_texture != nullptr);

    return depth_texture;
}

void UploadToBuffer(SDL_GPUCommandBuffer* command_buffer, SDL_GPUBuffer* buffer, std::span<BufferUploadData> upload_data) {
    auto& g = globals;

    struct BufferUploadSdlData {
        SDL_GPUBufferRegion           dst;
        SDL_GPUTransferBufferLocation src;
    };

    byte* mem = (byte*)SDL_MapGPUTransferBuffer(g.device, g.transfer_buffer, false);
    assert(mem != nullptr);

    std::vector<BufferUploadSdlData> uploads;
    uploads.reserve(upload_data.size());

    size_t offset = 0;
    for (const auto& data : upload_data) {
        assert(offset + data.data.size() <= SSBO_SIZE);
        memcpy(mem + offset, data.data.data(), data.data.size_bytes());

        BufferUploadSdlData upload = {};
        upload.src = { .transfer_buffer = g.transfer_buffer, .offset = (u32)offset };
        upload.dst = { .buffer = buffer, .offset = data.offset, .size = (u32)data.data.size() };
        uploads.push_back(upload);

        offset += data.data.size();
    }
    SDL_UnmapGPUTransferBuffer(g.device, g.transfer_buffer);

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    for (const auto& upload : uploads) {
        SDL_UploadToGPUBuffer(copy_pass, &upload.src, &upload.dst, false);
    }
    SDL_EndGPUCopyPass(copy_pass);
}

