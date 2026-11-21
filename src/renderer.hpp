#pragma once

#include "resources.hpp"

struct Renderer {
    Renderer(const GPUDevice& device);
    void RenderFrame();
private:
    SDL_GPUDevice* m_device = nullptr;

    GPUGraphicsPipeline m_box_pipeline;
    GPUGraphicsPipeline m_particles_pipeline;
};

