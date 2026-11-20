#pragma once

struct GPUResourceDeleter {
    SDL_GPUDevice* device = nullptr;

    void operator()(SDL_GPUDevice* ptr) const {
        SDL_DestroyGPUDevice(ptr);
    }
    void operator()(SDL_GPUBuffer* ptr) const {
        SDL_ReleaseGPUBuffer(device, ptr);
    }
    void operator()(SDL_GPUComputePipeline* ptr) const {
        SDL_ReleaseGPUComputePipeline(device, ptr);
    }
    void operator()(SDL_GPUGraphicsPipeline* ptr) const {
        SDL_ReleaseGPUGraphicsPipeline(device, ptr);
    }
    void operator()(SDL_GPUShader* ptr) const {
        SDL_ReleaseGPUShader(device, ptr);
    }
    void operator()(SDL_GPUTexture* ptr) const {
        SDL_ReleaseGPUTexture(device, ptr);
    }
    void operator()(SDL_GPUSampler* ptr) const {
        SDL_ReleaseGPUSampler(device, ptr);
    }
};

using GPUDevice           = std::unique_ptr<SDL_GPUDevice, GPUResourceDeleter>;
using GPUBuffer           = std::unique_ptr<SDL_GPUBuffer, GPUResourceDeleter>;
using GPUComputePipeline  = std::unique_ptr<SDL_GPUComputePipeline, GPUResourceDeleter>;
using GPUGraphicsPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline, GPUResourceDeleter>;
using GPUShader           = std::unique_ptr<SDL_GPUShader, GPUResourceDeleter>;
using GPUTexture          = std::unique_ptr<SDL_GPUTexture, GPUResourceDeleter>;
using GPUSampler          = std::unique_ptr<SDL_GPUSampler, GPUResourceDeleter>;

struct ReflectionData {
    u32 num_samplers;
    u32 num_readonly_storage_textures;
    u32 num_readonly_storage_buffers;
    u32 num_readwrite_storage_textures;
    u32 num_readwrite_storage_buffers;
    u32 num_uniform_buffers;
    u32 threadcount_x;
    u32 threadcount_y;
    u32 threadcount_z;
};

std::vector<u32> CompileShader(const std::string& path);
ReflectionData ReflectShader(std::span<u32> spirv_code);

GPUDevice CreateGPUDevice();
GPUBuffer CreateGPUBuffer(const GPUDevice& device, const SDL_GPUBufferCreateInfo& create_info);
GPUComputePipeline CreateGPUComputePipeline(const GPUDevice& device, const SDL_GPUComputePipelineCreateInfo& create_info);
GPUComputePipeline CreateGPUComputePipeline(const GPUDevice& device, const std::string& path);
GPUGraphicsPipeline CreateGPUGraphicsPipeline(const GPUDevice& device, const SDL_GPUGraphicsPipelineCreateInfo& create_info);
GPUShader CreateGPUShader(const GPUDevice& device, const SDL_GPUShaderCreateInfo& create_info);
GPUShader CreateGPUShader(const GPUDevice& device, const std::string& path);
GPUTexture CreateGPUTexture(const GPUDevice& device, const SDL_GPUTextureCreateInfo& create_info);
GPUSampler CreateGPUSampler(const GPUDevice& device, const SDL_GPUSamplerCreateInfo& create_info);

class GPUComputePipelineEx {
public:
    GPUComputePipelineEx(const GPUDevice& device, const std::string& path);

    void ReloadIfModified();
    void SetBindings(
        std::span<const SDL_GPUStorageTextureReadWriteBinding> textures,
        std::span<const SDL_GPUStorageBufferReadWriteBinding> buffers);
    void DispatchAsPass(SDL_GPUCommandBuffer* command_buffer, u32 work_x, u32 work_y, u32 work_z) const;

private:
    bool RebuildPipeline();

    SDL_GPUDevice* m_device;
    std::string m_path;
    GPUComputePipeline m_pipeline;
    std::filesystem::file_time_type m_last_write_time;

    std::vector<SDL_GPUStorageTextureReadWriteBinding> m_texture_bindings;
    std::vector<SDL_GPUStorageBufferReadWriteBinding> m_buffer_bindings;

    u32 m_thread_count_x = 1;
    u32 m_thread_count_y = 1;
    u32 m_thread_count_z = 1;
};

