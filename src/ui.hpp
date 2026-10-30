#pragma once

inline struct UIData {
    uint32_t  particle_num          = 0;
    float     timestep              = 0.01666f;
    float     speed                 = 1.0f;
    float     particle_radius       = 0.01f;
    float     grid_cell_size        = 2.2f * UIData::particle_radius;
    float     density               = 1000.0f;
    glm::vec3 box_size              = glm::vec3(2.0f, 2.0f, 2.0f);

    float     gravity               = -9.81f;
    int       substep_num           = 4;
    float     pic_flip              = 0.1f;
    int       collision_iter_num    = 4;
    int       pressure_iter_num     = 4;
    float     viscosity             = 0.1f;
    bool      compensate_drift      = true;
    bool      reset_simulation      = false;
} g_ui;

void UIDrawFrame();

