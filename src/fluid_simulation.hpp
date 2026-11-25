#pragma once
#include "resources.hpp"

struct FluidSimulation {
    static constexpr size_t PARTICLE_BYTE_SIZE =
        sizeof(vec3)  + // position
        sizeof(vec3)  + // velocity
        sizeof(vec3)  + // predicted_position
        sizeof(float) + // density
        sizeof(float) + // lambda
        sizeof(vec3)  + // vorticity
        sizeof(vec3);   // color
    static constexpr size_t PARTICLE_MAX_NUM = 100'000;
    static constexpr size_t SSBO_SIZE = PARTICLE_MAX_NUM * PARTICLE_BYTE_SIZE;

    // static constexpr float KERNEL_R =kk 
    static constexpr ivec3 GRID_MAX_SIZE = ivec3(64, 32, 64);

    static constexpr const char* RESET_PATH = "shaders/simulation/reset.comp";
    static constexpr const char* SORT_PATH = "shaders/simulation/sort.comp";

    static inline ShaderDefineVector DEFINES = {
        { "PARTICLE_NUM", std::to_string(PARTICLE_MAX_NUM) }
    };

    FluidSimulation(SDL_GPUDevice* device, SDL_GPUTransferBuffer* transfer_buffer, const ivec3& grid_size);
    void Reset(SDL_GPUCommandBuffer* command_buffer);
    void Run(SDL_GPUCommandBuffer* command_buffer);
private:
    // Non-owning
    SDL_GPUDevice*         m_device          = nullptr;
    SDL_GPUTransferBuffer* m_transfer_buffer = nullptr;

    GPUBuffer            m_particles;
    GPUBuffer            m_grid;
    GPUComputePipelineEx m_reset_pipeline;
    GPUComputePipelineEx m_sort_pipeline;

    u32 m_particle_num = 0;
    ivec3 m_grid_size;

    // Sets the defines and bindings
    GPUComputePipelineEx CreateSimulationCompute(const char* path);
    void DispatchCompute(const GPUComputePipelineEx& pipeline, SDL_GPUCommandBuffer* command_buffer);
};

