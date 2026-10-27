#pragma once

constexpr float FIXED_TIMESTEP = 0.01666f; // 60Hz

using UpdateFunc = std::function<void(float delta_time)>;

void MainLoop(UpdateFunc variable_update, UpdateFunc fixed_update);

