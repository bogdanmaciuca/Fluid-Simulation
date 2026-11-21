#version 450
#if defined(VS)
layout(location = 0) out vec2 v_uv;

void main() {
    vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    vec2 p = positions[gl_VertexIndex];
    v_uv = p * 0.5 + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}

#elif defined(FS)
layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(u_texture, v_uv);
}
#endif

