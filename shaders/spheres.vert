#version 450

// Per-instance attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragCoord;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4  mvp;
    float aspectRatio;
    float radius;
} ubo;

// Hardcoded quad vertices (2 triangles, 6 vertices total) ranging from -1 to 1
const vec2 QUAD_VERTICES[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

void main() {
    vec2 localPos = QUAD_VERTICES[gl_VertexIndex];
    fragCoord = localPos; // Pass unscaled local coordinates to fragment shader
    fragColor = inColor;

    vec4 centerClip = ubo.mvp * vec4(inPosition, 1.0);
    float radius = ubo.radius;

    // Scale the X coordinate by the inverse aspect ratio to keep the quad square on screen
    vec2 scale = vec2(1.0 / ubo.aspectRatio, 1.0);

    // Offset in clip space with aspect correction and perspective sizing
    gl_Position = centerClip + vec4(localPos * radius * scale, 0.0, 0.0);
}
