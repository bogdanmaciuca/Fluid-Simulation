#version 450

layout(set = 1, binding = 0) uniform UniformBlock {
    mat4  u_ModelViewProjection;
    vec2  u_ScreenResolution;
    float u_LineWidth;
};

// 8 unique corners of a unit cube [-0.5, 0.5]
const vec3 Corners[8] = vec3[8](
    vec3(-0.5, -0.5, -0.5), // 0
    vec3( 0.5, -0.5, -0.5), // 1
    vec3( 0.5,  0.5, -0.5), // 2
    vec3(-0.5,  0.5, -0.5), // 3
    vec3(-0.5, -0.5,  0.5), // 4
    vec3( 0.5, -0.5,  0.5), // 5
    vec3( 0.5,  0.5,  0.5), // 6
    vec3(-0.5,  0.5,  0.5)  // 7
);

// 12 edges without diagonals (2 corner indices each)
const uvec2 Edges[12] = uvec2[12](
    // Back face (4 edges)
    uvec2(0, 1), uvec2(1, 2), uvec2(2, 3), uvec2(3, 0),
    // Front face (4 edges)
    uvec2(4, 5), uvec2(5, 6), uvec2(6, 7), uvec2(7, 4),
    // Connecting edges (4 edges)
    uvec2(0, 4), uvec2(1, 5), uvec2(2, 6), uvec2(3, 7)
);

// 6 quad vertices (2 triangles):
// x in [0.0, 1.0] along the segment length
// y in [-1.0, 1.0] across the line thickness
const vec2 QuadVertices[6] = vec2[6](
    vec2(0.0, -1.0),
    vec2(1.0, -1.0),
    vec2(0.0,  1.0),
    vec2(0.0,  1.0),
    vec2(1.0, -1.0),
    vec2(1.0,  1.0)
);

void main()
{
    // Get the 3D start and end endpoints for this instance's edge
    uvec2 edge = Edges[gl_InstanceIndex];
    vec3 p0_world = Corners[edge.x];
    vec3 p1_world = Corners[edge.y];

    // Project both points into clip space
    vec4 clip0 = u_ModelViewProjection * vec4(p0_world, 1.0);
    vec4 clip1 = u_ModelViewProjection * vec4(p1_world, 1.0);

    // Screen coordinates (NDC -> Pixel space)
    vec2 ndc0 = clip0.xy / clip0.w;
    vec2 ndc1 = clip1.xy / clip1.w;

    vec2 screen0 = (ndc0 * 0.5 + 0.5) * u_ScreenResolution;
    vec2 screen1 = (ndc1 * 0.5 + 0.5) * u_ScreenResolution;

    // Compute 2D direction and perpendicular normal
    vec2 dir = screen1 - screen0;
    float len = length(dir);

    // Fallback normal if line faces directly along camera forward vector
    vec2 norm = (len > 0.0001) ? vec2(-dir.y, dir.x) / len : vec2(0.0);

    // Expand quad vertex
    vec2 quad = QuadVertices[gl_VertexIndex];

    // Choose the corresponding endpoint base (p0 if x == 0.0, p1 if x == 1.0)
    vec4 clipBase = (quad.x < 0.5) ? clip0 : clip1;

    // Extrude perpendicularly by half the line width (in pixels)
    vec2 screenOffset = norm * (quad.y * 0.5 * u_LineWidth);

    // Convert pixel offset back to clip space offset (scaled by perspective W)
    vec2 clipOffset = (screenOffset / u_ScreenResolution) * 2.0 * clipBase.w;

    clipBase.xy += clipOffset;
    gl_Position = clipBase;
}
