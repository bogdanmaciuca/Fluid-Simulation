#pragma once
#include "camera.hpp"
#include "../helpers.hpp"
#include "shapes.hpp"

struct Renderer {
    static constexpr float DEFAULT_FOV = 80.0f;
    static constexpr float MIN_FOV     = 10.0f;
    static constexpr float MAX_FOV     = 120.0f;
    static constexpr float ZOOM_SPEED  =  2.0f;

    static constexpr const char* GEOM_SHADER_PATH = "shaders/geometry.glsl";

    Renderer(SDL_GPUDevice* device, SDL_GPUTextureFormat render_target_format, SDL_GPUBuffer* fluid_buffer);

    void ProcessCameraInput(std::function<void(Camera& camera)> process_camera);
    void ProcessMouseScroll(float yoffset);

    void PushCone(const Cone& cone);
    void PushCylinder(const Cylinder& cylinder);
    void PushSphere(const Sphere& sphere);
    void PushAABB(const AABB& aabb);

    void RenderFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* output_texture, int texture_w, int texture_h);
private:
    // Non-owning
    SDL_GPUDevice* m_device;
    SDL_GPUBuffer* m_fluid_buffer;

    GPUTextureEx          m_depth_texture;
    GPUTransferBuffer     m_transfer_buffer;
    GPUBuffer             m_geometry_buffer;
    GPUGraphicsPipelineEx m_geometry_pipeline;

    Camera             m_camera;
    float              m_fov = DEFAULT_FOV;
    std::vector<Shape> m_shapes;
};

