#pragma once

// SSBO
// Arrays:
// - vec3  position
// - vec3  predicted_position
// - vec3  velocity
// - float density
// - float lambda
// - vec3  vorticity
// - vec3  color

inline struct Globals {
    SDL_Window*              window;
    SDL_GPUDevice*           device;
    SDL_GPUBuffer*           ssbo;
    SDL_GPUGraphicsPipeline* circles_pipeline;
    SDL_GPUTexture*          depth_texture;
    SDL_GPUGraphicsPipeline* box_pipeline;

    ImGuiIO* io;

    struct {
        bool pressed;
        bool down;
        bool released;
    } keys[SDL_SCANCODE_COUNT];
} globals;

constexpr size_t PARTICLE_BYTE_SIZE =
    sizeof(vec3)  + // position
    sizeof(vec3)  + // predicted_position
    sizeof(vec3)  + // velocity
    sizeof(float) + // density
    sizeof(float) + // lambda
    sizeof(vec3)  + // voriticity
    sizeof(vec3);   // color

constexpr size_t PARTICLE_MAX_NUM = 100'000;

constexpr size_t SSBO_SIZE = PARTICLE_MAX_NUM * PARTICLE_BYTE_SIZE;

// Shader paths
constexpr const char* FLUID_VS_PATH = "shaders/fluid.vert";
constexpr const char* FLUID_FS_PATH = "shaders/fluid.frag";
constexpr const char* BOX_VS_PATH   = "shaders/box.vert";
constexpr const char* BOX_FS_PATH   = "shaders/box.frag";

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

