cmake_minimum_required(VERSION 3.15)
project(Fluids LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRCS
    src/*.cpp
    vendor/**/*.cpp
)

include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)

# SDL3
set(SDL_SHARED OFF CACHE BOOL "Build SDL as static library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL as static library" FORCE)
set(SDL_TESTS  OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Disable SDL examples" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18
)
FetchContent_MakeAvailable(SDL3)

# GLSLang
set(ALLOW_TESTS OFF CACHE BOOL "Disable glslang tests" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable testing" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Skip building standalone glslangValidator/glslangang executables to save build time" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip installation rules for glslang" FORCE)
# Disable optimization and external SPIRV tools
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ALLOW_EXTERNAL_SPIRV_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG        main
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(glslang)

add_executable(${PROJECT_NAME} ${SRCS})

target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.hpp)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    FL_DEBUG=$<$<CONFIG:Debug>:1>$<$<NOT:$<CONFIG:Debug>>:0>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3
    glm::glm
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
)

target_include_directories(${PROJECT_NAME} PRIVATE vendor/imgui)


