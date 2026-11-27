#include "renderer.hpp"

constexpr size_t GEOMETRY_BUFFER_SIZE = 1024 * 1024;

Renderer::Renderer(
    SDL_GPUDevice* device,
    SDL_GPUTextureFormat render_target_format,
    SDL_GPUBuffer* fluid_buffer
) :
    m_device(device),
    m_fluid_buffer(fluid_buffer)
{
    m_depth_texture = GPUTextureEx::Create(m_device, {
        .format               = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width                = 1200,
        .height               = 800,
        .layer_count_or_depth = 1,
        .num_levels           = 1,
    }).value();

    m_geometry_buffer = CreateGPUBuffer(m_device, {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size  = GEOMETRY_BUFFER_SIZE
    }).value();

    SDL_GPUColorTargetDescription swapchain_color_target = {
        .format = render_target_format,
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op        = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op        = SDL_GPU_BLENDOP_ADD,
            .enable_blend          = true,
        }
    };
    m_geometry_pipeline = GPUGraphicsPipelineEx::Create(m_device, GEOM_SHADER_PATH, {
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = false,
            .enable_depth_write = false,
        },
        .target_info = {
            .color_target_descriptions = &swapchain_color_target,
            .num_color_targets = 1,
            .depth_stencil_format = m_depth_texture.GetCreateInfo().format,
            .has_depth_stencil_target = true,
        },
    }).value();

    m_transfer_buffer = CreateGPUTransferBuffer(m_device, {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size  = GEOMETRY_BUFFER_SIZE
    }).value();
}

void Renderer::ProcessCameraInput(std::function<void(Camera& camera)> process_camera) {
    process_camera(m_camera);
}

void Renderer::ProcessMouseScroll(float yoffset) {
    m_fov = std::clamp(m_fov - yoffset * ZOOM_SPEED, MIN_FOV, MAX_FOV);
}

void Renderer::PushCone(const Cone& cone) {
    m_shapes.push_back(cone);
}

void Renderer::PushCylinder(const Cylinder& cylinder) {
    m_shapes.push_back(cylinder);
}

void Renderer::PushSphere(const Sphere& sphere) {
    m_shapes.push_back(sphere);
}

void Renderer::PushAABB(const AABB& aabb) {
    m_shapes.push_back(aabb);
}


void Renderer::RenderFrame(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* output_texture, int texture_w, int texture_h) {
#ifdef FL_DEBUG
    m_geometry_pipeline.ReloadIfModified();
#endif

    static std::vector<GeometryVertex> m_geometry_vertices;
    m_geometry_vertices.clear();

    m_depth_texture.Resize(texture_w, texture_h);

    const vec3 camera_pos = m_camera.GetPosition();
    auto shape_position = [](const Shape& shape) -> vec3 {
        return std::visit(overloaded{
            [](const Cone& cone)     -> vec3 { return cone.origin; },
            [](const Cylinder& cyl)  -> vec3 { return cyl.origin; },
            [](const Sphere& sphere) -> vec3 { return sphere.origin; },
            [](const AABB& aabb)     -> vec3 { return (aabb.min + aabb.max) * 0.5f; },
        }, shape);
    };

    std::sort(m_shapes.begin(), m_shapes.end(), [&](const Shape& a, const Shape& b) {
        const vec3 da = shape_position(a) - camera_pos;
        const vec3 db = shape_position(b) - camera_pos;
        return glm::dot(da, da) > glm::dot(db, db);
    });

    for (const Shape& shape : m_shapes) {
        MakeShapeVertices(m_geometry_vertices, shape);
    }
    m_shapes.clear();

    if (!m_geometry_vertices.empty()) {

        u32 vertices_byte_size = sizeof(GeometryVertex) * m_geometry_vertices.size();

        // Copy to transfer buffer
        void* mem = SDL_MapGPUTransferBuffer(m_device, m_transfer_buffer.get(), true);
        memcpy(mem, m_geometry_vertices.data(), vertices_byte_size);
        SDL_UnmapGPUTransferBuffer(m_device, m_transfer_buffer.get());

        // Upload to GPU buffer
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        SDL_GPUTransferBufferLocation transfer_buffer_location = {
            .transfer_buffer = m_transfer_buffer.get(),
            .offset          = 0
        };
        SDL_GPUBufferRegion buffer_region = {
            .buffer = m_geometry_buffer.get(),
            .offset = 0,
            .size   = vertices_byte_size
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);
        SDL_EndGPUCopyPass(copy_pass);
    }

    // Push uniforms
    mat4 view_proj = glm::perspective(glm::radians(m_fov), (float)texture_w / texture_h, 0.001f, 100.0f) * m_camera.GetViewMatrix();
    SDL_PushGPUVertexUniformData(command_buffer, 0, &view_proj, sizeof(view_proj));

    // Render pass
    SDL_GPUColorTargetInfo color_target = {
        .texture     = output_texture,
        .clear_color = SDL_FColor{0.4, 0.6, 0.1, 1.0},
        .load_op     = SDL_GPU_LOADOP_CLEAR,
        .cycle       = true,
    };
    SDL_GPUDepthStencilTargetInfo depth_target = {
        .texture         = m_depth_texture.Get(),
        .clear_depth     = 1.0f,
        .load_op         = SDL_GPU_LOADOP_CLEAR,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .cycle           = true,
    };
    auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target, 1, &depth_target);
    if (!m_geometry_vertices.empty()) {
        SDL_BindGPUGraphicsPipeline(render_pass, m_geometry_pipeline.Get());
        SDL_GPUBufferBinding vertex_buffer_binding = {
            .buffer = m_geometry_buffer.get(),
            .offset = 0
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
        SDL_DrawGPUPrimitives(render_pass, m_geometry_vertices.size(), 1, 0, 0);
    }
    SDL_EndGPURenderPass(render_pass);

    m_geometry_vertices.clear();
}
