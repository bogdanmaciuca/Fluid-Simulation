#include "fluid_simulation.hpp"

namespace {
}

// Should be std140 layout
struct ComputeUBO {
    alignas(16) vec3  flow;
    alignas(16) vec3  wind;
    alignas(16) vec3  gravity;
    alignas(16) vec3  dims;
    alignas(4)  float rho0;
    alignas(4)  float viscosity;
    alignas(4)  u32   particle_num;
};

struct ResetUBO {
    alignas(16) ivec3 grid_size;
};

FluidSimulation::FluidSimulation(
    SDL_GPUDevice* device,
    SDL_GPUTransferBuffer* transfer_buffer,
    const ivec3& grid_size
) :
    m_device(device),
    m_transfer_buffer(transfer_buffer)
{
    // Particles
    SDL_GPUBufferUsageFlags particles_usage =
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ |
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE |
        SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    m_particles = CreateGPUBuffer(m_device, { .usage = particles_usage, .size = SSBO_SIZE }).value();

    // Grid
    SDL_GPUBufferUsageFlags grid_usage =
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ |
        SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    m_grid = CreateGPUBuffer(m_device, { .usage = grid_usage, .size = SSBO_SIZE }).value();

    // Reset
    m_reset_pipeline = GPUComputePipelineEx::Create(m_device, RESET_PATH, DEFINES).value();

    // Sort
    m_sort_pipeline = GPUComputePipelineEx::Create(m_device, SORT_PATH, DEFINES).value();
    m_sort_pipeline.SetBindings({}, std::array{
        SDL_GPUStorageBufferReadWriteBinding{ .buffer = m_particles.get() },
        SDL_GPUStorageBufferReadWriteBinding{ .buffer = m_particles.get() },
    });
}

void FluidSimulation::Reset(SDL_GPUCommandBuffer* command_buffer) {
    // ResetUBO
    // SDL_PushGPUFragmentUniformData(command_buffer, 0, );
    DispatchCompute(m_reset_pipeline, command_buffer);
}

void FluidSimulation::Run(SDL_GPUCommandBuffer* command_buffer) {
    m_sort_pipeline.DispatchAsPass(command_buffer, m_particle_num, 1, 1);
}

void FluidSimulation::DispatchCompute(const GPUComputePipelineEx& pipeline, SDL_GPUCommandBuffer* command_buffer) {
    pipeline.DispatchAsPass(command_buffer, m_particle_num, 1, 1);
}

