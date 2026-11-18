#ifndef SIM_UBO_GLSL
#define SIM_UBO_GLSL

layout(std140, set = 2, binding = 0) uniform SimUBO {
    vec3  flow;
    vec3  wind;
    vec3  gravity;
    vec3  dims;
    float rho0;
    float viscosity;
    uint  particle_num;
};

#endif

