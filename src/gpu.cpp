#include "gpu.hpp"

namespace {
    SDL_GPUDevice* s_device;
    SDL_Window*    s_window_ptr;
}

gpu::Context::Context(SDL_Window* window) {
    s_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    SDL_ClaimWindowForGPUDevice(s_device, window);

    glslang::InitializeProcess();
}

gpu::Context::~Context() {
    glslang::FinalizeProcess();

    SDL_ReleaseWindowFromGPUDevice(s_device, s_window_ptr);
    SDL_DestroyGPUDevice(s_device);
}

gpu::Buffer::Buffer(const SDL_GPUBufferCreateInfo& create_info) {
    m_ptr = SDL_CreateGPUBuffer(s_device, &create_info);
    m_size = create_info.size;
    assert(m_ptr != nullptr);
}

gpu::Buffer::~Buffer() {
    if (m_ptr != nullptr) {
        SDL_ReleaseGPUBuffer(s_device, m_ptr);
    }
}

gpu::Buffer::Buffer(Buffer&& other) {
    m_ptr        = other.m_ptr;
    m_size       = other.m_size;
    other.m_ptr  = nullptr;
    other.m_size = 0;
}

gpu::Buffer& gpu::Buffer::operator=(Buffer&& other) {
    if (&other != this) {
        if (m_ptr != nullptr) {
            SDL_ReleaseGPUBuffer(s_device, m_ptr);
        }
        m_ptr        = other.m_ptr;
        m_size       = other.m_size;
        other.m_ptr  = nullptr;
        other.m_size = 0;
    }
    return *this;
}


void gpu::Buffer::Resize(u32 new_size) {
    if (new_size <= m_size) {
        return;
    }

    SDL_GPUBufferCreateInfo create_info = { .usage = m_usage, .size = new_size };
    SDL_GPUBuffer* new_buffer = SDL_CreateGPUBuffer(s_device, &create_info);
    assert(new_buffer != nullptr);
    m_ptr = new_buffer;
}

gpu::Texture::Texture(const SDL_GPUTextureCreateInfo& create_info) {
    m_ptr = SDL_CreateGPUTexture(s_device, &create_info);
    m_size = create_info.size;
    assert(m_ptr != nullptr);
}

gpu::Texture::~Texture() {
    if (m_ptr != nullptr) {
        SDL_ReleaseGPUTexture(s_device, m_ptr);
    }
}

gpu::Texture::Texture(Texture&& other) {
    m_ptr        = other.m_ptr;
    m_size       = other.m_size;
    other.m_ptr  = nullptr;
    other.m_size = 0;
}

gpu::Texture& gpu::Texture::operator=(Texture&& other) {
    if (&other != this) {
        if (m_ptr != nullptr) {
            SDL_ReleaseGPUTexture(s_device, m_ptr);
        }
        m_ptr        = other.m_ptr;
        m_size       = other.m_size;
        other.m_ptr  = nullptr;
        other.m_size = 0;
    }
    return *this;
}


void gpu::Texture::Resize(u32 new_size) {
    if (new_size <= m_size) {
        return;
    }

    SDL_GPUTextureCreateInfo create_info = { .usage = m_usage, .size = new_size };
    SDL_GPUTexture* new_buffer = SDL_CreateGPUTexture(s_device, &create_info);
    assert(new_buffer != nullptr);
    m_ptr = new_buffer;
}
