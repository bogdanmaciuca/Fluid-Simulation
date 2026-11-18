#pragma once

constexpr float FIXED_TIMESTEP = 0.01666f; // 60Hz

using UpdateFunc = std::function<void(float delta_time)>;

struct MainLoopCallbacks {
    std::function<void(float delta_time)>       variable_update;
    std::function<void(float timestep)>         fixed_update;
    std::function<bool(const SDL_Event& event)> on_event;
};

void MainLoop(const MainLoopCallbacks& callbacks);

