#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <span>
#include <vector>

#include "SDL3/SDL.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_sdl3.h"
#include "../vendor/imgui/backends/imgui_impl_sdlgpu3.h"

#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/SPIRV/GlslangToSpv.h"

#include "glm/glm.hpp"

