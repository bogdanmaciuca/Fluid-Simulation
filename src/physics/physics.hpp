#pragma once


void PhysicsUpdate();
void PhysicsInit();
std::span<glm::vec3> PhysicsGetPositions();
std::span<glm::vec3> Colors();


std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> GenerateGrid(
    const std::vector<glm::vec3>& positions,
    const glm::ivec3& cell_num,
    float inv_spacing
);