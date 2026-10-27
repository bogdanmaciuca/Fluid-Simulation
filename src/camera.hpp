#pragma once

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right
};

constexpr float DEFAULT_YAW         = -90.0f;
constexpr float DEFAULT_PITCH       =  0.0f;
constexpr float DEFAULT_SPEED       =  2.5f;
constexpr float DEFAULT_SENSITIVITY =  0.1f;
constexpr float DEFAULT_ZOOM        =  45.0f;

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = DEFAULT_YAW, float pitch = DEFAULT_PITCH);

    void ProcessKeyboard(CameraMovement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

    glm::mat4 GetViewMatrix() const;

    float GetZoom() const { return m_zoom; }
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetFront() const { return m_front; }

private:
    void UpdateCameraVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;

    float m_yaw;
    float m_pitch;

    float m_movement_speed;
    float m_mouse_sensitivity;
    float m_zoom;
};

