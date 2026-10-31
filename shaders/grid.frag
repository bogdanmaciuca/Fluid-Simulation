#version 450

layout(location = 0) in float v_Color;

layout(location = 0) out vec4 outColor;

void main() {
    const float alpha = 0.2;

    outColor = vec4(mix(vec3(0.2, 1, 0.2), vec3(1, 0.2, 0.2), v_Color), alpha);
}


