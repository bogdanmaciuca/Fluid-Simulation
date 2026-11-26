#include "util.hpp"

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());

    std::ostringstream ss;
    ss << file.rdbuf();
    assert(!file.bad());

    return ss.str();
}

vec2 GetMouseDelta() {
    static bool has_last_position = false;
    static vec2 last_position     = vec2(0.0f);

    float x = 0.0f;
    float y = 0.0f;
    SDL_GetMouseState(&x, &y);

    const vec2 current_position(x, y);
    const vec2 delta = has_last_position
        ? current_position - last_position
        : vec2(0.0f);

    last_position     = current_position;
    has_last_position = true;

    return delta;
}

bool LeftMouseButtonDown() {
    auto flags = SDL_GetMouseState(nullptr, nullptr);
    return flags & SDL_BUTTON_LMASK;
}

