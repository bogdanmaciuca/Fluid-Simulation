#include "camera.hpp"
#include "main_loop.hpp"
#include "renderer.hpp"
#include "ui.hpp"

namespace {
    constexpr int   CUBE_GRID_DIM = 5;
    constexpr float CUBE_SIZE     = 1.0f / CUBE_GRID_DIM;

    std::vector<glm::vec4> BuildCubeGrid() {
        std::vector<glm::vec4> grid_cells;
        grid_cells.reserve(CUBE_GRID_DIM * CUBE_GRID_DIM * CUBE_GRID_DIM);

        const float half_extent = CUBE_SIZE * CUBE_GRID_DIM * 0.5f;
        const float color_span  = 3.0f * (CUBE_GRID_DIM - 1);

        for (int z = CUBE_GRID_DIM / 2; z < CUBE_GRID_DIM; ++z) {
            for (int y = 2; y < CUBE_GRID_DIM; ++y) {
                for (int x = 0; x < CUBE_GRID_DIM; ++x) {
                    glm::vec3 center(
                        x * CUBE_SIZE - half_extent + CUBE_SIZE * 0.5f,
                        y * CUBE_SIZE - half_extent + CUBE_SIZE * 0.5f,
                        z * CUBE_SIZE - half_extent + CUBE_SIZE * 0.5f
                    );
                    float color_mix = color_span > 0.0f ? (x + y + z) / color_span : 0.0f;
                    grid_cells.emplace_back(center, color_mix);
                }
            }
        }

        return grid_cells;
    }
}

int main() {
    bool renderer_init_result = RendererInitialize();
    assert(renderer_init_result);

    RendererImGui(UIDrawFrame);

    const bool* keyboard = SDL_GetKeyboardState(nullptr);

    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    const std::vector<glm::vec4> grid_cells = BuildCubeGrid();

    const auto variable_update = [&](float delta_time) {
        // Process mouse movement for camera only if LMB is down
        float dx, dy;
        SDL_MouseButtonFlags mouse_flags = SDL_GetRelativeMouseState(&dx, &dy);
        if (mouse_flags & SDL_BUTTON_LMASK) {
            camera.ProcessMouseMovement(dx, -dy);
        }

        if      (keyboard[SDL_SCANCODE_W]) { camera.ProcessKeyboard(CameraMovement::Forward,  delta_time); }
        else if (keyboard[SDL_SCANCODE_S]) { camera.ProcessKeyboard(CameraMovement::Backward, delta_time); }
        if      (keyboard[SDL_SCANCODE_A]) { camera.ProcessKeyboard(CameraMovement::Left,     delta_time); }
        else if (keyboard[SDL_SCANCODE_D]) { camera.ProcessKeyboard(CameraMovement::Right,    delta_time); }

        RendererSetViewMatrix(camera.GetViewMatrix());
        RendererDrawFrame({
            .grid_cells       = grid_cells,
            .sphere_positions = {},
            .sphere_colors    = {},
            .grid_cell_num    = glm::vec3(CUBE_GRID_DIM),
        });
    };

    const auto fixed_update = [&](float timestep) {
    };

    MainLoop(variable_update, fixed_update);

    RendererShutdown();
    return 0;
}
