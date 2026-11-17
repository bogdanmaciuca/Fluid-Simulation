#include "gpu.hpp"

gpu::Context::Context(SDL_Window* window) {
    assert(window != nullptr);
    s_window_ptr = window;

    s_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    assert(s_device != nullptr);
    bool claimed = SDL_ClaimWindowForGPUDevice(s_device, window);
    assert(claimed);

    bool glslang_init = glslang::InitializeProcess();
    assert(glslang_init);
}

gpu::Context::~Context() {
    glslang::FinalizeProcess();

    SDL_ReleaseWindowFromGPUDevice(s_device, s_window_ptr);
    SDL_DestroyGPUDevice(s_device);
}

void gpu::TransferBuffer::Map(std::function<void(void* mem)> f) {
    void* mem = SDL_MapGPUTransferBuffer(GetDevice(), m_ptr, false);
    f(mem);
    SDL_UnmapGPUTransferBuffer(GetDevice(), m_ptr);
}

