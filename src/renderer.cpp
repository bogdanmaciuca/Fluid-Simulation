#include "renderer.hpp"

namespace {
    SDL_Window*    s_window;
    SDL_GPUDevice* s_device;

    ImGuiIO* s_io;
    std::function<void(ImGuiIO&)> s_imgui_func;
}

bool RendererInitialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    s_window = SDL_CreateWindow("Fluid Simulation", DEFAULT_WND_WIDTH, DEFAULT_WND_HEIGHT, 0);
    if (!s_window) {
        printf("Failed to create window: %s", SDL_GetError());
        return false;
    }

    s_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, FL_DEBUG, nullptr);
    if (!s_device) {
        printf("Failed to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(s_device, s_window)) {
        printf("Failed to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    s_io = &ImGui::GetIO();
    s_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(s_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = s_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(s_device, s_window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    return true;
}

void RendererShutdown() {
    SDL_WaitForGPUIdle(s_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(s_device, s_window);
    SDL_DestroyGPUDevice(s_device);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}

void RendererUploadSpheres(std::span<const glm::vec3> positions, std::span<const glm::vec3> colors) {

}

void RendererImGui(std::function<void(ImGuiIO&)> f) {
    s_imgui_func = f;
}

void RendererDrawFrame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        // ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / s_io->Framerate, s_io->Framerate);
        ImGui::End();
    }

    // // 3. Show another simple window.
    // {
    //     ImGui::Text("Hello from another window!");
    //     ImGui::End();
    // }

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);


    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(s_device);

    SDL_GPUTexture* swapchain_texture;
    uint32_t swapchain_texture_width, swapchain_texture_height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, s_window, &swapchain_texture, &swapchain_texture_width, &swapchain_texture_height)) {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
    }

    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

    SDL_GPUColorTargetInfo swapchain_color_target = {
        .texture     = swapchain_texture,
        .clear_color = SDL_FColor{0.2f, 0.4f, 0.6f, 1.0f},
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .store_op    = SDL_GPU_STOREOP_STORE,
        .cycle       = true
    };
    std::array color_target_infos{ swapchain_color_target };


    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, color_target_infos.data(), color_target_infos.size(), nullptr);
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
    SDL_EndGPURenderPass(render_pass);

    SDL_SubmitGPUCommandBuffer(command_buffer);
}

