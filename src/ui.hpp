#pragma once

inline struct UIData {
    float     timestep            = 0.01666f;
    float     speed               = 1.0f;
    float     particle_separation = 0.01f;
    glm::vec3 box_size            = glm::vec3(2.5f, 1.0f, 1.2f);

    float     gravity             = -9.81f;
    float     density             = 1000.0f;
    float     pic_flip            = 0.1f;
} g_ui;

void UIDrawFrame();

