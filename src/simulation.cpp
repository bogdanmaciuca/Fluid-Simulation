#include "simulation.hpp"

// Should be std140 layout
struct UBO {
    alignas(16) vec3  flow;
    alignas(16) vec3  wind;
    alignas(16) vec3  gravity;
    alignas(16) vec3  dims;
    alignas(4)  float rho0;
    alignas(4)  float viscosity;
    alignas(4)  u32   particle_num;
};

Simulation::Simulation(SDL_GPUDevice* device) :
    m_device(device)
{
    SDL_GPUBufferUsageFlags usage =
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ |
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE |
        SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    m_ssbo = CreateGPUBuffer(m_device, { .usage = usage, .size = SSBO_SIZE });

    m_sort_pipeline = CreateGPUComputePipelineEx(device, SORT_PATH);
}

