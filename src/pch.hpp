#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "SDL3/SDL.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_sdl3.h"
#include "../vendor/imgui/backends/imgui_impl_sdlgpu3.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "spirv_reflect.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

using u8   = std::uint8_t;
using u16  = std::uint16_t;
using u32  = std::uint32_t;
using u64  = std::uint64_t;
using i8   = std::int8_t;
using i16  = std::int16_t;
using i32  = std::int32_t;
using i64  = std::int64_t;
using f32  = float;
using f64  = double;
using byte = std::uint8_t;

using vec2  = glm::vec2;
using vec3  = glm::vec3;
using vec4  = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;
using mat3  = glm::mat3;
using mat4  = glm::mat4;

template<typename... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
