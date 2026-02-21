#include "Camera.h"
#include <QtMath>
#include <algorithm>

Camera::Camera()
    : m_pitch(45.0f)
    , m_yaw(0.0f)
    , m_radius(300.0f)
    , m_fov(60.0f)
    , m_aspectRatio(1.33f)
    , m_zNear(1.0f)
    , m_zFar(1000.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_worldUp(0.0f, 1.0f, 0.0f)
{
    updateViewMatrix();
    updateProjectionMatrix();
}

void Camera::rotate(float dPitch, float dYaw) {
    m_pitch += dPitch;
    m_yaw += dYaw;

    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < 25.0f) m_pitch = 25.0f;

    updateViewMatrix();
}

void Camera::zoom(float amount) {
    m_radius -= amount;

    if (m_radius < 100.0f) m_radius = 100.0f;
    if (m_radius > 500.0f) m_radius = 500.0f;

    updateViewMatrix();
}

void Camera::setAspectRatio(float aspect) {
    if (std::abs(m_aspectRatio - aspect) > 0.001f) {
        m_aspectRatio = aspect;
        updateProjectionMatrix();
    }
}

const QMatrix4x4& Camera::getViewMatrix() const {
    return m_viewMatrix;
}

const QMatrix4x4& Camera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

QMatrix4x4 Camera::getViewProjectionMatrix() const {
    return m_projectionMatrix * m_viewMatrix;
}

QVector3D Camera::getPosition() const {
    return m_position;
}

void Camera::updateViewMatrix() {
    float radPitch = qDegreesToRadians(m_pitch);
    float radYaw = qDegreesToRadians(m_yaw);

    // Сферические координаты -> Декартовы
    float x = std::sin(radYaw) * std::cos(radPitch) * m_radius;
    float y = std::sin(radPitch) * m_radius;
    float z = std::cos(radYaw) * std::cos(radPitch) * m_radius;

    m_position = QVector3D(x, y, z);

    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt(m_position, m_target, m_worldUp);
}

void Camera::updateProjectionMatrix() {
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(m_fov, m_aspectRatio, m_zNear, m_zFar);
}

QVector3D Camera::getRay(float screenX, float screenY, float screenWidth, float screenHeight) const {
    // NDC: [-1, 1]
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / screenHeight;

    QVector3D f = (m_target - m_position).normalized(); // Forward
    QVector3D s = QVector3D::crossProduct(f, m_worldUp).normalized(); // Side (Right)
    QVector3D u = QVector3D::crossProduct(s, f); // Up (Local)

    // Размеры Viewport на расстоянии 1 от камеры
    float viewH = 2.0f * std::tan(qDegreesToRadians(m_fov) / 2.0f);
    float viewW = viewH * m_aspectRatio;

    return (f + s * (x * viewW / 2.0f) + u * (y * viewH / 2.0f)).normalized();
}
