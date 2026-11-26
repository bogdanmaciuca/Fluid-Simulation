#pragma once

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right
};

constexpr float DEFAULT_YAW         = -90.0;
constexpr float DEFAULT_PITCH       =  0.0;
constexpr float DEFAULT_SPEED       =  2.5;
constexpr float DEFAULT_SENSITIVITY =  0.1;

class Camera {
public:
    Camera(vec3 position = vec3(0.0, 0.0, 0.0), vec3 up = vec3(0.0, 1.0, 0.0), float yaw = DEFAULT_YAW, float pitch = DEFAULT_PITCH);

    void ProcessKeyboard(CameraMovement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    mat4 GetViewMatrix() const;

    vec3 GetPosition() const { return m_position; }
    vec3 GetFront() const { return m_front; }

private:
    void UpdateCameraVectors();

    vec3 m_position;
    vec3 m_front;
    vec3 m_up;
    vec3 m_right;
    vec3 m_world_up;

    float m_yaw;
    float m_pitch;

    float m_movement_speed;
    float m_mouse_sensitivity;
};

