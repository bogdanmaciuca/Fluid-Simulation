#version 450

layout(location = 0) in vec4 a_Cube; // xyz = position, w = color mix
layout(location = 0) out float v_Color;

layout(set = 1, binding = 0) uniform UniformBlock {
    mat4  u_ModelViewProjection;
    vec2  u_ScreenResolution;
    float u_CubeSize;
};

// 6 faces, 2 triangles per face, 3 vertices per triangle = 36 vertices.
const vec3 FaceVertices[36] = vec3[36](
    // +Z
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5), vec3( 0.5,  0.5,  0.5),
    vec3(-0.5, -0.5,  0.5), vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    // -Z
    vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5, -0.5), vec3( 0.5,  0.5, -0.5),
    vec3(-0.5, -0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5, -0.5, -0.5),
    // +X
    vec3( 0.5, -0.5, -0.5), vec3( 0.5,  0.5, -0.5), vec3( 0.5,  0.5,  0.5),
    vec3( 0.5, -0.5, -0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5, -0.5,  0.5),
    // -X
    vec3(-0.5, -0.5, -0.5), vec3(-0.5, -0.5,  0.5), vec3(-0.5,  0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3(-0.5,  0.5,  0.5), vec3(-0.5,  0.5, -0.5),
    // +Y
    vec3(-0.5,  0.5, -0.5), vec3(-0.5,  0.5,  0.5), vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5, -0.5), vec3( 0.5,  0.5,  0.5), vec3( 0.5,  0.5, -0.5),
    // -Y
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5), vec3( 0.5, -0.5,  0.5),
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5,  0.5), vec3(-0.5, -0.5,  0.5)
);

void main()
{
    int vertexIndex = gl_VertexIndex;

    // The previous wireframe shader used 72 vertices per cube (12 edges * 6).
    // Clip anything beyond the 36 solid-face vertices so existing draw calls
    // that still submit 72 vertices do not draw duplicate geometry.
    if (vertexIndex >= 36) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        v_Color = 0.0;
        return;
    }

    vec3 position = FaceVertices[vertexIndex] * u_CubeSize + a_Cube.xyz;
    gl_Position = u_ModelViewProjection * vec4(position, 1.0);

    v_Color = a_Cube.w;
}
