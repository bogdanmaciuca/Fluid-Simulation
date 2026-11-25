#pragma once

constexpr float FIXED_TIMESTEP = 0.01666f; // 60Hz

using UpdateFunc = std::function<void(float dt)>;
using RenderFunc = std::function<void()>;
using OnEventFunc = std::function<bool(const SDL_Event& event)>;

struct MainLoopCallbacks {
    OnEventFunc on_event;
    UpdateFunc  fixed_update;
    UpdateFunc  variable_update;
};

void MainLoop(const MainLoopCallbacks& callbacks);

