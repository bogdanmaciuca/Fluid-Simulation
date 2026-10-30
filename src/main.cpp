#include "camera.hpp"
#include "main_loop.hpp"
#include "physics.hpp"
#include "renderer.hpp"
#include "ui.hpp"

int main() {
    bool renderer_init_result = RendererInitialize();
    assert(renderer_init_result);

    RendererImGui(UIDrawFrame);

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    g_ui.box_size           = glm::vec3(0.5f);
    g_ui.substep_num        = 4;
    g_ui.collision_iter_num = 2;
    g_ui.pressure_iter_num  = 10;

    Physics physics;

    const glm::vec3 obstacle_position(-100.0f);
    const float     obstacle_radius = 0.15f;

    Camera camera(glm::vec3(
        g_ui.box_size.x * 0.5f,
        g_ui.box_size.y * 0.5f,
        g_ui.box_size.z * 2.5f
    ));

    const auto variable_update = [&](float delta_time) {
        // Process mouse movement for camera only if LMB is down and ImGui is not
        // using the mouse (e.g. dragging a slider or hovering a window).
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

        g_ui.particle_num = physics.ParticleNum();

        RendererSetViewMatrix(camera.GetViewMatrix());
        RendererDrawFrame(physics.ParticlePositions(), physics.ParticleColors());
    };

    const auto fixed_update = [&](float timestep) {
        if (g_ui.reset_simulation) {
            physics.Reset();
            g_ui.reset_simulation = false;
        }

        physics.Update(timestep, obstacle_position, obstacle_radius);
    };

    MainLoop(variable_update, fixed_update);

    RendererShutdown();
    return 0;
}

