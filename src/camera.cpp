#include "camera.hpp"

Camera::Camera(glm::dvec3 position, glm::dvec3 up, double yaw, double pitch)
    : m_front(glm::dvec3(0.0, 0.0, -1.0)), m_movement_speed(DEFAULT_SPEED), m_mouse_sensitivity(DEFAULT_SENSITIVITY), m_zoom(DEFAULT_ZOOM)
{
    m_position = position;
    m_world_up = up;
    m_yaw = yaw;
    m_pitch = pitch;
    UpdateCameraVectors();
}

void Camera::ProcessKeyboard(CameraMovement direction, double deltaTime) {
    double velocity = m_movement_speed * deltaTime;
    if (direction == CameraMovement::Forward)
        m_position += m_front * velocity;
    if (direction == CameraMovement::Backward)
        m_position -= m_front * velocity;
    if (direction == CameraMovement::Left)
        m_position -= m_right * velocity;
    if (direction == CameraMovement::Right)
        m_position += m_right * velocity;
}

void Camera::ProcessMouseMovement(double xoffset, double yoffset, bool constrainPitch) {
    xoffset *= m_mouse_sensitivity;
    yoffset *= m_mouse_sensitivity;

    m_yaw   += xoffset;
    m_pitch += yoffset;

    if (m_pitch > 89.0)
        m_pitch = 89.0;
    if (m_pitch < -89.0)
        m_pitch = -89.0;

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(double yoffset) {
    m_zoom -= yoffset;
    if (m_zoom < 1.0)
        m_zoom = 1.0;
    if (m_zoom > 45.0)
        m_zoom = 45.0;
}

glm::dmat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::UpdateCameraVectors() {
    glm::dvec3 front;
    front.x = cos(glm::radians<double>(m_yaw)) * cos(glm::radians<double>(m_pitch));
    front.y = sin(glm::radians<double>(m_pitch));
    front.z = sin(glm::radians<double>(m_yaw)) * cos(glm::radians<double>(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

