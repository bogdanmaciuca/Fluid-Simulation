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
    void operator()(SDL_GPUTransferBuffer* ptr) const {
        SDL_ReleaseGPUTransferBuffer(device, ptr);
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
using GPUTransferBuffer   = std::unique_ptr<SDL_GPUTransferBuffer, ResourceDeleter>;
using GPUSampler          = std::unique_ptr<SDL_GPUSampler, ResourceDeleter>;

//
// Shader utilities; mainly used internally
//
struct ReflectionData {
    u32 num_samplers                   = 0;
    u32 num_readonly_storage_textures  = 0;
    u32 num_readonly_storage_buffers   = 0;
    u32 num_readwrite_storage_textures = 0;
    u32 num_readwrite_storage_buffers  = 0;
    u32 num_uniform_buffers            = 0;
    u32 threadcount_x                  = 0;
    u32 threadcount_y                  = 0;
    u32 threadcount_z                  = 0;
    std::vector<SDL_GPUVertexAttribute> vertex_attributes;
};

using ShaderDefine       = std::pair<std::string, std::string>;
using ShaderDefineVector = std::vector<ShaderDefine>;

std::optional<std::vector<u32>> CompileShader(const std::string& path, EShLanguage stage, const ShaderDefineVector& defines);
std::optional<ReflectionData> ReflectShader(std::span<u32> spirv_code);

//
// Create* functions for the basic resources we wrapped
//
std::optional<GPUBuffer> CreateGPUBuffer(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo& create_info);
std::optional<GPUComputePipeline> CreateGPUComputePipeline(SDL_GPUDevice* device, const SDL_GPUComputePipelineCreateInfo& create_info);
std::optional<GPUComputePipeline> CreateGPUComputePipeline(SDL_GPUDevice* device, const std::string& path, ShaderDefineVector defines = {}, ReflectionData* reflection = nullptr);
std::optional<GPUDevice> CreateGPUDevice();
std::optional<GPUGraphicsPipeline> CreateGPUGraphicsPipeline(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& create_info);
std::optional<GPUSampler> CreateGPUSampler(SDL_GPUDevice* device, const SDL_GPUSamplerCreateInfo& create_info);
std::optional<GPUShader> CreateGPUShader(SDL_GPUDevice* device, const SDL_GPUShaderCreateInfo& create_info, const ShaderDefineVector& defines = {});
std::optional<GPUShader> CreateGPUShader(SDL_GPUDevice* device, const std::string& path, const ShaderDefineVector& defines = {}, ReflectionData* reflection = nullptr);
std::optional<GPUShader> CreateGPUShaderFragment(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection = nullptr);
std::optional<GPUShader> CreateGPUShaderVertex(SDL_GPUDevice* device, const std::string& path, ReflectionData* reflection = nullptr);
std::optional<GPUTexture> CreateGPUTexture(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info);
std::optional<GPUTransferBuffer> CreateGPUTransferBuffer(SDL_GPUDevice* device, const SDL_GPUTransferBufferCreateInfo& create_info);
std::optional<Window> CreateWindow(const std::string& title, int w, int h);

template<typename T>
struct GPUResourceEx {
    bool IsValid() const { return (bool)m_res; }
    [[nodiscard]] auto Get() const { return m_res.get(); }

protected:
    void SetDevice(SDL_GPUDevice* device) { m_res.get_deleter().device = device; }
    SDL_GPUDevice* Device() const { return m_res.get_deleter().device; }
    void Set(T&& resource) { m_res = std::move(resource); }

    T m_res;
};

//
// Extented resources; these encapsulate a basic resource + some metadata and
// functionality
//
struct GPUComputePipelineEx : GPUResourceEx<GPUComputePipeline> {
    static std::optional<GPUComputePipelineEx> Create(
        SDL_GPUDevice* device,
        const std::string& path,
        const ShaderDefineVector& defines
    );

    bool ReloadIfModified();
    void SetBindings(
        std::span<const SDL_GPUStorageTextureReadWriteBinding> textures,
        std::span<const SDL_GPUStorageBufferReadWriteBinding> buffers);
    bool SetDefines(ShaderDefineVector defines, bool rebuild = true);
    void DispatchAsPass(SDL_GPUCommandBuffer* command_buffer, u32 work_x, u32 work_y, u32 work_z) const;
private:
    bool RebuildPipeline();

    std::string                     m_path;
    std::filesystem::file_time_type m_last_write_time;

    std::vector<SDL_GPUStorageTextureReadWriteBinding> m_texture_bindings;
    std::vector<SDL_GPUStorageBufferReadWriteBinding>  m_buffer_bindings;

    ShaderDefineVector m_defines;

    u32 m_thread_count_x = 1;
    u32 m_thread_count_y = 1;
    u32 m_thread_count_z = 1;
};

struct GPUGraphicsPipelineEx : GPUResourceEx<GPUGraphicsPipeline> {
    // The `vertex_shader`, `fragment_shader` and `vertex_input_state` fields
    // of `SDL_GPUGraphicsPipelineCreateInfo` are automatically filled via
    // shader compilation/reflection.
    static std::optional<GPUGraphicsPipelineEx> Create(
        SDL_GPUDevice* device,
        const std::string& path,
        const SDL_GPUGraphicsPipelineCreateInfo& create_info
    );

    bool ReloadIfModified();
private:
    bool RebuildPipeline();

    std::string m_path;
    SDL_GPUGraphicsPipelineCreateInfo m_create_info;
    std::filesystem::file_time_type m_last_write_time;
    std::vector<SDL_GPUColorTargetDescription>  m_color_targets;
    std::vector<SDL_GPUVertexBufferDescription> m_vertex_buffers;
};

struct GPUTextureEx : GPUResourceEx<GPUTexture> {
    static std::optional<GPUTextureEx> Create(SDL_GPUDevice* device, const SDL_GPUTextureCreateInfo& create_info);

    // Resizes the texture if one of the dimensions is less than the required
    // ones. All data gets erased.
    bool EnsureSize(u32 width, u32 height, u32 depth = 1);
    // Resizes the texture if the dimensions don't match exactly. All data gets
    // erased.
    bool Resize(u32 width, u32 height, u32 depth = 1);
    auto GetCreateInfo() const { return m_create_info; }
private:
    bool Recreate(u32 width, u32 height, u32 depth);

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
    void ControlPanelAndScene(GPUTextureEx& scene_texture, std::function<void()> control_panel, bool* scene_is_focused = nullptr);
    void DispatchRenderPass(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* texture, bool clear = false);
private:
    SDL_GPUDevice* m_device;

    ImGuiIO* m_io;
};

