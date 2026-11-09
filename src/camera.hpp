#pragma once

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right
};

constexpr double DEFAULT_YAW         = -90.0;
constexpr double DEFAULT_PITCH       =  0.0;
constexpr double DEFAULT_SPEED       =  2.5;
constexpr double DEFAULT_SENSITIVITY =  0.1;
constexpr double DEFAULT_ZOOM        =  45.0;

class Camera {
public:
    Camera(glm::dvec3 position = glm::dvec3(0.0, 0.0, 0.0), glm::dvec3 up = glm::dvec3(0.0, 1.0, 0.0), double yaw = DEFAULT_YAW, double pitch = DEFAULT_PITCH);

    void ProcessKeyboard(CameraMovement direction, double deltaTime);
    void ProcessMouseMovement(double xoffset, double yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(double yoffset);

    glm::dmat4 GetViewMatrix() const;

    double GetZoom() const { return m_zoom; }
    glm::dvec3 GetPosition() const { return m_position; }
    glm::dvec3 GetFront() const { return m_front; }

private:
    void UpdateCameraVectors();

    glm::dvec3 m_position;
    glm::dvec3 m_front;
    glm::dvec3 m_up;
    glm::dvec3 m_right;
    glm::dvec3 m_world_up;

    double m_yaw;
    double m_pitch;

    double m_movement_speed;
    double m_mouse_sensitivity;
    double m_zoom;
};

