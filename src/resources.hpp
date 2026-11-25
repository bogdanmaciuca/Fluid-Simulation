#pragma once

//
// Basic resources; thin SDL_* wrappers
//
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

//
// Shader utilities; mainly used internally
//
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
    std::vector<SDL_GPUVertexAttribute> vertex_attributes;
};

using ShaderDefine       = std::pair<std::string, std::string>;
using ShaderDefineVector = std::vector<ShaderDefine>;

std::optional<std::vector<u32>> CompileShader(const std::string& path, EShLanguage stage, const ShaderDefineVector& defines);
std::optional<ReflectionData> ReflectShader(std::span<u32> spirv_code);

//
// Create* functions for the basic resources we wrapped
//
std::optional<Window> CreateWindow(const std::string& title, int w, int h);

std::optional<GPUDevice> CreateGPUDevice();

std::optional<GPUBuffer> CreateGPUBuffer(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo& create_info);

std::optional<GPUComputePipeline> CreateGPUComputePipeline(SDL_GPUDevice* device, const SDL_GPUComputePipelineCreateInfo& create_info);
std::optional<GPUComputePipeline> CreateGPUComputePipeline(SDL_GPUDevice* device, const std::string& path, ShaderDefineVector defines = {}, ReflectionData* reflection = nullptr);

std::optional<GPUGraphicsPipeline> CreateGPUGraphicsPipeline(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& create_info);

std::optional<GPUShader> CreateGPUShader(SDL_GPUDevice* device, const SDL_GPUShaderCreateInfo& create_info, const ShaderDefineVector& defines = {});
std::optional<GPUShader> CreateGPUShader(SDL_GPUDevice* device, const std::string& path, const ShaderDefineVector& defines = {}, ReflectionData* reflection = nullptr);
std::optional<GPUShader> CreateGPUShaderVertex(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection = nullptr);
std::optional<GPUShader> CreateGPUShaderFragment(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection = nullptr);

std::optional<GPUTexture> CreateGPUTexture(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info);
std::optional<GPUSampler> CreateGPUSampler(SDL_GPUDevice* device, const SDL_GPUSamplerCreateInfo& create_info);

//
// Extented resources; these encapsulate a basic resource + some metadata and
// functionality
//
struct GPUComputePipelineEx {
    GPUComputePipelineEx() = default;
    GPUComputePipelineEx(
        SDL_GPUDevice* device,
        const std::string& path,
        const ShaderDefineVector& defines = {}
    );

    void ReloadIfModified();
    void SetBindings(
        std::span<const SDL_GPUStorageTextureReadWriteBinding> textures,
        std::span<const SDL_GPUStorageBufferReadWriteBinding> buffers);
    void SetDefines(ShaderDefineVector defines);
    void DispatchAsPass(SDL_GPUCommandBuffer* command_buffer, u32 work_x, u32 work_y, u32 work_z) const;
    auto Get() { return m_pipeline.get(); }
private:
    bool RebuildPipeline();

    std::string                     m_path;
    GPUComputePipeline              m_pipeline;
    std::filesystem::file_time_type m_last_write_time;

    std::vector<SDL_GPUStorageTextureReadWriteBinding> m_texture_bindings;
    std::vector<SDL_GPUStorageBufferReadWriteBinding>  m_buffer_bindings;

    ShaderDefineVector m_defines;

    u32 m_thread_count_x = 1;
    u32 m_thread_count_y = 1;
    u32 m_thread_count_z = 1;
};

struct GPUGraphicsPipelineEx {
    GPUGraphicsPipelineEx() = default;
    // The `vertex_shader`, `fragment_shader` and `vertex_input_state` fields
    // of `SDL_GPUGraphicsPipelineCreateInfo` are automatically filled via
    // shader compilation/reflection.
    GPUGraphicsPipelineEx(SDL_GPUDevice* device, const std::string& path, const SDL_GPUGraphicsPipelineCreateInfo& create_info);

    void ReloadIfModified();
    auto Get() { return m_pipeline.get(); }
private:
    bool RebuildPipeline();

    std::string m_path;
    GPUGraphicsPipeline m_pipeline;
    SDL_GPUGraphicsPipelineCreateInfo m_create_info;
    std::filesystem::file_time_type m_last_write_time;
};

struct GPUTextureEx {
    GPUTextureEx(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info);

    // Resizes the texture if one of the dimensions is less than the required
    // ones. All data gets erased.
    void EnsureSize(u32 width, u32 height, u32 depth = 1);
    // Resizes the texture if the dimensions don't match exactly. All data gets
    // erased.
    void Resize(u32 width, u32 height, u32 depth = 1);
    auto Get() { return m_texture.get(); }
    auto GetCreateInfo() const { return m_create_info; }
private:
    GPUTexture m_texture;
    SDL_GPUTextureCreateInfo m_create_info;
};

//
// ImGui RAII helper. Must be defined only once and passed by reference if needed.
// Eliminates some boilerplate
//
struct ImGuiHelper {
    ImGuiHelper(SDL_GPUDevice* device, SDL_Window* window);
    ~ImGuiHelper();
    ImGuiHelper(const ImGuiHelper&) = delete;
    ImGuiHelper& operator=(const ImGuiHelper&) = delete;
    ImGuiHelper(ImGuiHelper&&) = delete;
    ImGuiHelper& operator=(ImGuiHelper&&) = delete;

    void NewFrame();
    void EndFrame();
    void ControlPanelAndScene(GPUTextureEx& scene_texture, std::function<void(float, float)> control_panel);
    void DispatchRenderPass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* texture, bool clear = false);
private:
    SDL_GPUDevice* m_device;

    ImGuiIO* m_io;
};


