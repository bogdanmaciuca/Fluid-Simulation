#pragma once
#include "resources.hpp"

constexpr size_t PARTICLE_BYTE_SIZE =
    sizeof(vec3)  + // position
    sizeof(vec3)  + // predicted_position
    sizeof(vec3)  + // velocity
    sizeof(float) + // density
    sizeof(float) + // lambda
    sizeof(vec3)  + // vorticity
    sizeof(vec3);   // color
constexpr size_t PARTICLE_MAX_NUM = 100'000;
constexpr size_t SSBO_SIZE = PARTICLE_MAX_NUM * PARTICLE_BYTE_SIZE;

// Placeholder
constexpr const char* SORT_PATH = "shaders/simulation/sort.comp";

struct Simulation {
    Simulation(SDL_GPUDevice* device);
    void Run();
private:
    SDL_GPUDevice* m_device = nullptr;

    GPUBuffer m_ssbo;
    GPUComputePipelineEx m_sort_pipeline;
};

