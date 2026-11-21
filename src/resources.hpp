#pragma once

struct ResourceDeleter {
    SDL_GPUDevice* device = nullptr;

    void operator()(SDL_Window* ptr) const {
        SDL_DestroyWindow(ptr);
    }
    void operator()(SDL_GPUDevice* ptr) const {
        SDL_WaitForGPUIdle(ptr);
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

using Window              = std::unique_ptr<SDL_Window, ResourceDeleter>;
using GPUDevice           = std::unique_ptr<SDL_GPUDevice, ResourceDeleter>;
using GPUBuffer           = std::unique_ptr<SDL_GPUBuffer, ResourceDeleter>;
using GPUComputePipeline  = std::unique_ptr<SDL_GPUComputePipeline, ResourceDeleter>;
using GPUGraphicsPipeline = std::unique_ptr<SDL_GPUGraphicsPipeline, ResourceDeleter>;
using GPUShader           = std::unique_ptr<SDL_GPUShader, ResourceDeleter>;
using GPUTexture          = std::unique_ptr<SDL_GPUTexture, ResourceDeleter>;
using GPUSampler          = std::unique_ptr<SDL_GPUSampler, ResourceDeleter>;

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

Window CreateWindow(const std::string& title, int w, int h);
GPUDevice CreateGPUDevice();
GPUBuffer CreateGPUBuffer(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo& create_info);
GPUComputePipeline CreateGPUComputePipeline(SDL_GPUDevice* device, const SDL_GPUComputePipelineCreateInfo& create_info);
GPUComputePipeline CreateGPUComputePipeline(SDL_GPUDevice* device, const std::string& path);
GPUGraphicsPipeline CreateGPUGraphicsPipeline(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& create_info);
GPUShader CreateGPUShader(SDL_GPUDevice* device, const SDL_GPUShaderCreateInfo& create_info);
GPUShader CreateGPUShader(SDL_GPUDevice* device, const std::string& path);
GPUTexture CreateGPUTexture(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info);
GPUSampler CreateGPUSampler(SDL_GPUDevice* device, const SDL_GPUSamplerCreateInfo& create_info);

class GPUComputePipelineEx {
public:
    GPUComputePipelineEx() = default;
    GPUComputePipelineEx(SDL_GPUDevice* device, const std::string& path);

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

GPUComputePipelineEx CreateGPUComputePipelineEx(SDL_GPUDevice* device, const std::string& path);

