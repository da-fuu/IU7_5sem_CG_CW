#ifndef CAMERA_H
#define CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>

class Camera {
public:
    Camera();

    // Управление
    void rotate(float dPitch, float dYaw);
    void zoom(float amount);
    void setAspectRatio(float aspect);

    // Геттеры для рендеринга
    const QMatrix4x4& getViewMatrix() const;
    const QMatrix4x4& getProjectionMatrix() const;
    QMatrix4x4 getViewProjectionMatrix() const;
    QVector3D getPosition() const;

    // Генерация луча для кликов мыши
    QVector3D getRay(float screenX, float screenY, float screenWidth, float screenHeight) const;

private:
    void updateViewMatrix();
    void updateProjectionMatrix();

    // Параметры камеры
    float m_pitch;
    float m_yaw;
    float m_radius;

    // Параметры проекции
    float m_fov;
    float m_aspectRatio;
    float m_zNear;
    float m_zFar;

    // Кэшированные матрицы и векторы
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_projectionMatrix;
    QVector3D m_position;
    QVector3D m_target;
    QVector3D m_worldUp;
};

#endif // CAMERA_H
