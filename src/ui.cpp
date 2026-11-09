#include "ui.hpp"
#include "timer.hpp"

void UIDrawFrame() {
    auto& io = ImGui::GetIO();

    auto SliderAsMilli = [](const char* label, double* base_val, double min_milli, double max_milli) {
        double milli = *base_val * 1000.0;
        bool changed = ImGui::SliderScalar(label, ImGuiDataType_Double, &milli, &min_milli, &max_milli);
        if (changed) {
            *base_val = milli * 0.001;
        }
        return changed;
    };

    ImGui::Begin("Control Panel");

    ImGui::Checkbox("OOP", &g_ui.oop);
    ImGui::Text("Particles: %u", g_ui.particle_num);
    ImGui::Separator();

    {
        double speed_min = 0.01, speed_max = 2.0;
        ImGui::SliderScalar("Speed", ImGuiDataType_Double, &g_ui.speed, &speed_min, &speed_max);
    }
    SliderAsMilli("Particle radius (mm)", &g_ui.particle_radius, 1.0, 25.0);
    {
        double box_min = 1.0f, box_max = 2.0;
        ImGui::SliderScalarN("Box size (m)", ImGuiDataType_Double, &g_ui.box_size.x, 3, &box_min, &box_max);
    }
    {
        double g_min = -50.0, g_max = 50.0;
        ImGui::SliderScalar("Gravity (m^2/s)", ImGuiDataType_Double, &g_ui.gravity, &g_min, &g_max);
    }
    {
        double v_min = 0.0, v_max = 1.0;
        ImGui::SliderScalar("Viscosity", ImGuiDataType_Double, &g_ui.viscosity, &v_min, &v_max);
    }
    {
        double r_min = 0.0, r_max = 1.0;
        ImGui::SliderScalar("Restitution", ImGuiDataType_Double, &g_ui.restitution, &r_min, &r_max);
    }

    ImGui::Separator();
    ImGui::Text("Timings (avg ms)");
    for (const auto& [name, stats] : timers::Entries()) {
        ImGui::Text("%.*s: %.3f", (int)name.size(), name.data(), stats.avg_seconds * 1000.0);
    }

    ImGui::End();
}

