#ifndef SIM_SSBO_GLSL
#define SIM_SSBO_GLSL

// Source of truth for the maximum number of particles (the host)
#ifndef PARTICLE_MAX_NUM
#error "PARTICLE_MAX_NUM must me defined by the host"
#endif

layout(std430, set = 1, binding = 0) buffer particles {
    vec3  pos[PARTICLE_MAX_NUM];
    vec3  ppos[PARTICLE_MAX_NUM];
    vec3  vel[PARTICLE_MAX_NUM];
    float rho[PARTICLE_MAX_NUM];
    float lambda[PARTICLE_MAX_NUM];
    float omega[PARTICLE_MAX_NUM];
};

#endif

