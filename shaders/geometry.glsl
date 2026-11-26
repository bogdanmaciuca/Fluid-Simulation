#version 450

#if defined(VS)
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_color;

layout(location = 0) out vec3 f_color;
layout(location = 1) out vec3 f_barycentric;

layout(set = 1, binding = 0) uniform Camera {
    mat4 view_proj;
};

void main() {
    int corner = gl_VertexIndex % 3;
    f_barycentric = vec3(
        corner == 0 ? 1.0 : 0.0,
        corner == 1 ? 1.0 : 0.0,
        corner == 2 ? 1.0 : 0.0
    );
    f_color = v_color;
    gl_Position = view_proj * vec4(v_position, 1.0);
}

#elif defined(FS)
layout(location = 0) in vec3 f_color;
layout(location = 1) in vec3 f_barycentric;

layout(location = 0) out vec4 out_color;

const float EDGE_THICKNESS = 2.0;
const float EDGE_DARKNESS  = 0.10;
const float OPACITY = 0.5;

void main() {
    vec3 d        = fwidth(f_barycentric);
    vec3 smoothed = smoothstep(vec3(0.0), d * EDGE_THICKNESS, f_barycentric);
    float edge    = min(min(smoothed.x, smoothed.y), smoothed.z);
    float shade   = mix(1.0 - EDGE_DARKNESS, 1.0, edge);
    out_color = vec4(f_color * shade, OPACITY);
}
#endif
