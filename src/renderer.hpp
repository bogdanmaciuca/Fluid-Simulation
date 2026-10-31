#pragma once

constexpr int DEFAULT_WND_WIDTH = 1200;
constexpr int DEFAULT_WND_HEIGHT = 675;

bool RendererInitialize();
void RendererShutdown();

void RendererSetViewMatrix(const glm::mat4& view);
void RendererImGui(std::function<void()> f);

struct RendererFrameData {
    std::span<const glm::vec4> grid_cells;
    std::span<const glm::vec3> sphere_positions;
    std::span<const glm::vec3> sphere_colors;
    glm::vec3                  grid_cell_num = glm::vec3(0.0f);
};

void RendererDrawFrame(const RendererFrameData& frame_data);

