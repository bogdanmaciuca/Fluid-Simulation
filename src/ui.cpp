#include "ui.hpp"
#include "timer.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace {
    constexpr float PI = 3.14159265358979323846f;

    struct PieSlice {
        const char* label;
        float       milliseconds;
        ImU32       color;
    };

    float TimerMs(std::string_view name) {
        const auto& entries = timers::Entries();
        auto it = entries.find(name);
        return it == entries.end() ? 0.0f : static_cast<float>(it->second.last_seconds * 1000.0);
    }

    void DrawPieChart(const char* id, const PieSlice* slices, int count, float radius) {
        float total = 0.0f;
        for (int i = 0; i < count; i++) {
            total += std::max(0.0f, slices[i].milliseconds);
        }

        ImGui::PushID(id);

        if (total <= 0.0f) {
            ImGui::TextUnformatted("No data yet");
            ImGui::PopID();
            return;
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const ImVec2 center(cursor.x + radius, cursor.y + radius);

        ImGui::Dummy(ImVec2(radius * 2.0f, radius * 2.0f));

        float angle = -PI * 0.5f;
        for (int i = 0; i < count; i++) {
            const float value = std::max(0.0f, slices[i].milliseconds);
            if (value <= 0.0f) {
                continue;
            }

            const float next = angle + (value / total) * 2.0f * PI;
            const int segments = std::max(2, static_cast<int>((next - angle) / (2.0f * PI) * 64.0f));

            for (int s = 0; s < segments; s++) {
                const float a0 = angle + (next - angle) * (static_cast<float>(s) / segments);
                const float a1 = angle + (next - angle) * (static_cast<float>(s + 1) / segments);
                const ImVec2 p0(center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius);
                const ImVec2 p1(center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius);
                draw->AddTriangleFilled(center, p0, p1, slices[i].color);
            }

            angle = next;
        }

        draw->AddCircle(center, radius, IM_COL32(255, 255, 255, 80), 64, 1.0f);

        for (int i = 0; i < count; i++) {
            const float value = std::max(0.0f, slices[i].milliseconds);
            const float percent = value / total * 100.0f;

            ImGui::PushID(i);
            ImGui::ColorButton(
                slices[i].label,
                ImGui::ColorConvertU32ToFloat4(slices[i].color),
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs,
                ImVec2(12.0f, 12.0f)
            );
            ImGui::SameLine();
            ImGui::Text("%s: %.2f ms (%.0f%%)", slices[i].label, value, percent);
            ImGui::PopID();
        }

        ImGui::PopID();
    }

    void DrawTimers() {
        const float frame_ms   = TimerMs("Frame");
        const float physics_ms = TimerMs("Physics");
        const float render_ms  = TimerMs("Render");

        const float integrate_ms = TimerMs("Physics/Integrate");
        const float push_ms      = TimerMs("Physics/PushApart");
        const float collision_ms = TimerMs("Physics/Collisions");
        const float to_grid_ms   = TimerMs("Physics/TransferToGrid");
        const float density_ms   = TimerMs("Physics/Density");
        const float pressure_ms  = TimerMs("Physics/PressureSolve");
        const float from_grid_ms = TimerMs("Physics/TransferFromGrid");
        const float colors_ms    = TimerMs("Physics/Colors");

        const float phases_ms = integrate_ms + push_ms + collision_ms + to_grid_ms
                              + density_ms + pressure_ms + from_grid_ms + colors_ms;

        PieSlice slices[9] = {
            {"Integrate",   integrate_ms, IM_COL32(80, 160, 255, 255)},
            {"Push apart",  push_ms,      IM_COL32(240, 160, 70, 255)},
            {"Collisions",  collision_ms, IM_COL32(200, 180, 90, 255)},
            {"To grid",     to_grid_ms,   IM_COL32(80, 210, 140, 255)},
            {"Density",     density_ms,   IM_COL32(120, 200, 220, 255)},
            {"Pressure",    pressure_ms,  IM_COL32(200, 120, 220, 255)},
            {"From grid",   from_grid_ms, IM_COL32(150, 140, 240, 255)},
            {"Colors",      colors_ms,    IM_COL32(220, 140, 140, 255)},
            {"Other",       std::max(0.0f, physics_ms - phases_ms), IM_COL32(160, 160, 160, 255)},
        };

        ImGui::SeparatorText("Timers");
        ImGui::Text(
            "Frame %.2f ms | Physics %.2f ms | Render %.2f ms",
            frame_ms, physics_ms, render_ms
        );

        DrawPieChart("Physics breakdown", slices, 9, 74.0f);

        ImGui::SeparatorText("All timers");

        const auto& entries = timers::Entries();
        if (ImGui::BeginTable(
                "timer_table",
                4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp
            )) {
            ImGui::TableSetupColumn("Timer");
            ImGui::TableSetupColumn("Last (ms)");
            ImGui::TableSetupColumn("Avg (ms)");
            ImGui::TableSetupColumn("Calls");
            ImGui::TableHeadersRow();

            for (const auto& [name, stats] : entries) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(name.data(), name.data() + name.size());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.3f", stats.last_seconds * 1000.0);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", stats.avg_seconds * 1000.0);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%llu", static_cast<unsigned long long>(stats.last_calls));
            }

            ImGui::EndTable();
        }
    }
}

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

    ImGui::SetNextWindowSize(ImVec2(620.0f, 700.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Control Panel");

    if (ImGui::Button("Reset simulation")) {
        g_ui.reset_simulation = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset stats")) {
        timers::Reset();
    }

    ImGui::Text("Particles: %u", g_ui.particle_num);
    ImGui::Separator();

    SliderAsMilli("Timestep (ms)", &g_ui.timestep, 1.0f, 50.0f);
    ImGui::SliderFloat("Speed", &g_ui.speed, 0.01f, 2.0f);
    SliderAsMilli("Particle radius (mm)", &g_ui.particle_radius, 1.0f, 25.0f);
    SliderAsMilli("Grid cell size (mm)", &g_ui.grid_cell_size, 5.0f, 50.0f);
    ImGui::SliderFloat("Density (kg/m^3)", &g_ui.density, 1.0f, 1000.0f);
    ImGui::SliderFloat3("Box size (m)", &g_ui.box_size.x, 0.25f, 1.0f);

    ImGui::SliderFloat("Gravity (m^2/s)", &g_ui.gravity, -50.0f, 50.0f);
    ImGui::SliderInt("Substeps", &g_ui.substep_num, 1, 10);
    ImGui::SliderFloat("PIC/FLIP", &g_ui.pic_flip, 0.0f, 1.0f);
    ImGui::SliderInt("Collision iterations", &g_ui.collision_iter_num, 1, 10);
    ImGui::SliderInt("Pressure iterations", &g_ui.pressure_iter_num, 1, 50);
    ImGui::SliderFloat("Viscosity", &g_ui.viscosity, 0.0f, 1.0f);
    ImGui::Checkbox("Compensate drift", &g_ui.compensate_drift);

    DrawTimers();

    ImGui::End();
}

