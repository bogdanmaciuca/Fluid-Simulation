#pragma once

bool RendererInitialize();
void RendererShutdown();

void RenderGrid();
void RenderSpheres(std::span<const glm::vec3> positions, std::span<const glm::vec3> colors);

