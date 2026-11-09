#include "camera.hpp"
#include "main_loop.hpp"
#include "physics/physics.hpp"
#include "renderer.hpp"
#include "ui.hpp"

int main() {
    bool renderer_init_result = RendererInitialize();
    assert(renderer_init_result);

    RendererImGui(UIDrawFrame);

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    PhysicsInit();

    const glm::dvec3 obstacle_position(-100.0);
    const double     obstacle_radius = 0.15;

    Camera camera(glm::dvec3(
        g_ui.box_size.x * 0.5,
        g_ui.box_size.y * 0.5,
        g_ui.box_size.z * 2.5
    ));

    const auto variable_update = [&](double delta_time) {
        // Process mouse movement for camera only if LMB is down and ImGui is
        // not using the mouse
        float dx, dy;
        SDL_MouseButtonFlags mouse_flags = SDL_GetRelativeMouseState(&dx, &dy);
        if (!ImGui::GetIO().WantCaptureMouse && (mouse_flags & SDL_BUTTON_LMASK)) {
            camera.ProcessMouseMovement(dx, -dy);
            // ProcessMouseScroll(float yoffset); // TODO maybe
        }

        if      (keyboard[SDL_SCANCODE_W]) { camera.ProcessKeyboard(CameraMovement::Forward,  delta_time); }
        else if (keyboard[SDL_SCANCODE_S]) { camera.ProcessKeyboard(CameraMovement::Backward, delta_time); }
        if      (keyboard[SDL_SCANCODE_A]) { camera.ProcessKeyboard(CameraMovement::Left,     delta_time); }
        else if (keyboard[SDL_SCANCODE_D]) { camera.ProcessKeyboard(CameraMovement::Right,    delta_time); }

        // g_ui.particle_num = PhysicsGetParticleNum();

        RendererSetViewMatrix(camera.GetViewMatrix());
        RendererDrawFrame(PhysicsGetPositions(), PhysicsGetColors());
    };

    const auto fixed_update = [&](double timestep) {
        constexpr int NUM_SUBSTEPS = 10;
        constexpr double SUBSTEP = FIXED_TIMESTEP / (double)NUM_SUBSTEPS;
        for (int i = 0; i < NUM_SUBSTEPS; i++) {
            PhysicsUpdate(SUBSTEP);
        }
    };

    MainLoop(variable_update, fixed_update);

    RendererShutdown();
    return 0;
}

