#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <vector>

#include <glm/glm.hpp>

void PhysicsUpdate();
void PhysicsInit();

constexpr std::size_t MAX_PARTICLE_NUM = 1'000'000;

std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> GenerateGrid(
    const std::vector<glm::vec3>& positions,
    const glm::ivec3& cell_num,
    float inv_spacing
);