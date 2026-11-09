#pragma once

inline struct UIData {
    uint32_t   particle_num    = 0;
    float      grid_cube_size  = 0.1f;

    double     speed           = 1.0;
    double     particle_radius = 0.01;
    glm::dvec3 box_size        = glm::dvec3(1.0, 1.0, 1.0);
    double     gravity         = 0.0; //-9.81;
    double     viscosity       = 0.1;
    double     restitution     = 0.5;
    bool       oop             = false;
} g_ui;

void UIDrawFrame();

