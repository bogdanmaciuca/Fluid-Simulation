#pragma once

constexpr double FIXED_TIMESTEP = 0.01666; // 60Hz

using UpdateFunc = std::function<void(double delta_time)>;

void MainLoop(UpdateFunc variable_update, UpdateFunc fixed_update);

