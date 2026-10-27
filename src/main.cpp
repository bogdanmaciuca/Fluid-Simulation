#include "camera.hpp"
#include "main_loop.hpp"
#include "renderer.hpp"
#include "ui.hpp"

int main() {
    bool renderer_init_result = RendererInitialize();
    assert(renderer_init_result);

    RendererImGui(UIDrawFrame);

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    std::vector<glm::vec3> positions = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
    };
    std::vector<glm::vec3> colors = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };

    Camera camera;

    const auto variable_update = [&](float delta_time) {
        // Process mouse movement for camera only if LMB is down
        float dx, dy;
        SDL_MouseButtonFlags mouse_flags = SDL_GetRelativeMouseState(&dx, &dy);
        if (mouse_flags & SDL_BUTTON_LMASK) {
            camera.ProcessMouseMovement(dx, -dy);
            // ProcessMouseScroll(float yoffset); // TODO maybe
        }

        if      (keyboard[SDL_SCANCODE_W]) { camera.ProcessKeyboard(CameraMovement::Forward,  delta_time); }
        else if (keyboard[SDL_SCANCODE_S]) { camera.ProcessKeyboard(CameraMovement::Backward, delta_time); }
        if      (keyboard[SDL_SCANCODE_A]) { camera.ProcessKeyboard(CameraMovement::Left,     delta_time); }
        else if (keyboard[SDL_SCANCODE_D]) { camera.ProcessKeyboard(CameraMovement::Right,    delta_time); }

        RendererSetViewMatrix(camera.GetViewMatrix());
        RendererDrawFrame(positions, colors);
    };

    const auto fixed_update = [&](float timestep) {

    };

    MainLoop(variable_update, fixed_update);

    RendererShutdown();
    return 0;
}

