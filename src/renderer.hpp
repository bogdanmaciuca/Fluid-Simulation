#pragma once

constexpr int DEFAULT_WND_WIDTH = 1200;
constexpr int DEFAULT_WND_HEIGHT = 675;

bool RendererInitialize();
void RendererShutdown();

void RendererSetViewMatrix(const glm::dmat4& view);
void RendererImGui(std::function<void()> f);

struct RendererFrameData {
    std::span<const glm::vec4>  grid_cells;
    std::span<const glm::dvec3> sphere_positions;
    std::span<const glm::dvec3> sphere_colors;
};

void RendererDrawFrame(const RendererFrameData& frame_data);

