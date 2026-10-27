#pragma once

constexpr int DEFAULT_WND_WIDTH = 1200;
constexpr int DEFAULT_WND_HEIGHT = 675;

bool RendererInitialize();
void RendererShutdown();

void RendererSetViewMatrix(const glm::mat4& view);
void RendererImGui(std::function<void(ImGuiIO&)> f);
void RendererDrawFrame(
    std::span<const glm::vec3> sphere_positions,
    std::span<const glm::vec4> sphere_colors
);

