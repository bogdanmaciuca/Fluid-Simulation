#pragma once

void PhysicsInit();
void PhysicsUpdate(double dt);
uint32_t PhysicsGetParticleNum();
std::span<glm::dvec3> PhysicsGetPositions();
std::span<glm::dvec3> PhysicsGetColors();
std::span<glm::vec4> PhysicsGetGridCells();

