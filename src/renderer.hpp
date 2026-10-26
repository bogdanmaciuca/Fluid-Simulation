#pragma once

constexpr int DEFAULT_WND_WIDTH = 1200;
constexpr int DEFAULT_WND_HEIGHT = 675;

bool RendererInitialize();
void RendererShutdown();

void RendererUploadSpheres(std::span<const glm::vec3> positions, std::span<const glm::vec3> colors);
void RendererImGui(std::function<void(ImGuiIO&)> f);
void RendererDrawFrame();

