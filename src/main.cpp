#include "camera.hpp"
#include "main_loop.hpp"
#include "renderer.hpp"
#include "ui.hpp"

int main() {
    bool renderer_init_result = RendererInitialize();
    assert(renderer_init_result);

    RendererImGui(UIDrawFrame);

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;

    const int sizeX = 200;
    const int sizeY = 100;
    const int sizeZ = 100;
    const int totalPoints = sizeX * sizeY * sizeZ;

    // Clear initial elements and reserve memory for optimal performance
    positions.clear();
    colors.clear();
    positions.reserve(totalPoints);
    colors.reserve(totalPoints);

    auto hsvToRgb = [](float h, float s, float v) -> glm::vec3 {
        int i = static_cast<int>(h * 6.0f);
        float f = h * 6.0f - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        switch (i % 6) {
            case 0: return glm::vec3(v, t, p);
            case 1: return glm::vec3(q, v, p);
            case 2: return glm::vec3(p, v, t);
            case 3: return glm::vec3(p, q, v);
            case 4: return glm::vec3(t, p, v);
            case 5: return glm::vec3(v, p, q);
        }
        return glm::vec3(0.0f);
    };

    float stepSize = 0.15f;
    for (int z = 0; z < sizeZ; ++z) {
        for (int y = 0; y < sizeY; ++y) {
            for (int x = 0; x < sizeX; ++x) {
                positions.push_back(glm::vec3(
                    x * stepSize,
                    y * stepSize,
                    z * stepSize
                ));

                float hue = static_cast<float>(x + y * sizeX + z * sizeX * sizeY) / totalPoints;
                colors.push_back(glm::vec4(hsvToRgb(hue, 1.0f, 1.0f), 0.05f));
            }
        }
    }

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

