#include "ui.hpp"

void UIDrawFrame() {
    auto& io = ImGui::GetIO();

    auto SliderAsMilli = [](const char* label, float* base_val, float min_milli, float max_milli) {
        float milli = *base_val * 1000.0f;
        bool changed = ImGui::SliderFloat(label, &milli, min_milli, max_milli);
        if (changed) {
            *base_val = milli * 0.001f;
        }
        return changed;
    };

    ImGui::Begin("Control Panel");
    SliderAsMilli("Timestep (ms)", &g_ui.timestep, 1.0f, 50.0f);
    SliderAsMilli("Speed", &g_ui.speed, 0.01f, 2.0f);
    SliderAsMilli("Particle separation (mm)", &g_ui.particle_separation, 5.0f, 50.0f);
    ImGui::SliderFloat3("Box size (m)", &g_ui.box_size.x, 0.25f, 5.0f);
    ImGui::End();
}

