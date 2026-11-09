#include "physics.hpp"
#include "internal.hpp"
#include "../ui.hpp"

void PhysicsInit() {
    PhysicsDODInit();
    PhysicsOOPInit();
}

void PhysicsUpdate(double dt) {
    if (g_ui.oop) {
        PhysicsOOPUpdate(dt);
    }
    else {
        PhysicsDODUpdate(dt);
    }
}

std::span<glm::dvec3> PhysicsGetPositions() {
    if (g_ui.oop) {
        return PhysicsOOPGetPositions();
    }
    else {
        return PhysicsDODGetPositions();
    }
}

std::span<glm::dvec3> PhysicsGetColors() {
    if (g_ui.oop) {
        return PhysicsOOPGetColors();
    }
    else {
        return PhysicsDODGetColors();
    }
}

std::span<glm::vec4> PhysicsGetGridCells() {
    if (g_ui.oop) {
        return PhysicsOOPGetGridCells();
    }
    else {
        return PhysicsDODGetGridCells();
    }
}

