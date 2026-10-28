#version 450

layout(set = 3, binding = 0) uniform FragUniformBlock {
    vec4 u_LineColor;
};

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = u_LineColor;
}
