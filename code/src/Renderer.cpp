#include "Renderer.h"
#include <algorithm>
#include <limits>
#include <cmath>

Renderer::Renderer(int w, int h) {
    resize(w, h);
    lightDir = QVector3D(0.5f, 1.5f, 0.5f).normalized();
}

void Renderer::resize(int w, int h) {
    if (width == w && height == h) return;
    width = w;
    height = h;
    frameBuffer = QImage(width, height, QImage::Format_RGB32);
    zBuffer.resize(width * height);
}

void Renderer::clear(QRgb color) {
    frameBuffer.fill(color);
    std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());
}

void Renderer::setCamera(const Camera& camera) {
    viewProjMatrix = camera.getViewProjectionMatrix();
    camPos = camera.getPosition();
}

bool Renderer::project(const QVector3D& p, QPointF& out) const {
    QVector4D pc = viewProjMatrix * QVector4D(p, 1.0f);
    if (pc.w() < 1.0f) return false;
    float invW = 1.0f / pc.w();
    out.setX((pc.x() * invW + 1.0f) * 0.5f * width);
    out.setY((1.0f - (pc.y() * invW + 1.0f) * 0.5f) * height);
    return true;
}

bool Renderer::zBufferTest(int x, int y, float z) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return z < zBuffer[y * width + x];
}

void Renderer::updateZBuffer(int x, int y, float z) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        zBuffer[y * width + x] = z;
    }
}

void Renderer::drawTriangle(const Triangle& tri, const QMatrix4x4& modelMatrix, bool isShadow, bool isGhost) {
    QMatrix4x4 mvp = viewProjMatrix * modelMatrix;

    RasterVertex rVerts[3];

    for (int i = 0; i < 3; ++i) {
        // Экранные координаты
        QVector4D clipPos = mvp * QVector4D(tri.v[i].pos, 1.0f);

        // Простое отсечение
        if (clipPos.w() < 0.1f) return;

        float invW = 1.0f / clipPos.w();
        rVerts[i].screenPos = QVector3D(
            (clipPos.x() * invW + 1.0f) * 0.5f * width,
            (1.0f - (clipPos.y() * invW + 1.0f) * 0.5f) * height,
            clipPos.z() * invW
        );

        rVerts[i].worldPos = (modelMatrix * QVector4D(tri.v[i].pos, 1.0f)).toVector3D();
        // Вращаем нормаль
        rVerts[i].normal = (modelMatrix * QVector4D(tri.v[i].normal, 0.0f)).toVector3D();
    }

    RenderSettings settings;
    settings.color = tri.color;
    settings.isShadow = isShadow;
    settings.isGhost = isGhost;

    rasterize(rVerts[0], rVerts[1], rVerts[2], settings);
}

QRgb Renderer::calculatePhongPixel(const QVector3D& fragPos, const QVector3D& normal, const RenderSettings& settings) {
    if (settings.isShadow) return qRgb(30, 30, 30);

    if (settings.isGhost) {
        return settings.color;
    }

    float ambientStrength = 0.3f;
    QVector3D ambient = ambientStrength * QVector3D(1, 1, 1);

    QVector3D norm = normal.normalized();
    QVector3D lightDirNorm = lightDir.normalized();
    float diff = std::max(0.0f, QVector3D::dotProduct(norm, lightDirNorm));
    QVector3D diffuse = diff * QVector3D(1, 1, 1);

    float specularStrength = 0.5f;
    QVector3D viewDir = (camPos - fragPos).normalized();
    QVector3D reflectDir = (-lightDirNorm - 2.0f * QVector3D::dotProduct(-lightDirNorm, norm) * norm).normalized();

    float spec = std::pow(std::max(0.0f, QVector3D::dotProduct(viewDir, reflectDir)), 32);
    QVector3D specular = specularStrength * spec * QVector3D(1, 1, 1);

    QVector3D result = (ambient + diffuse + specular);

    int r = std::clamp((int)(qRed(settings.color) * result.x()), 0, 255);
    int g = std::clamp((int)(qGreen(settings.color) * result.y()), 0, 255);
    int b = std::clamp((int)(qBlue(settings.color) * result.z()), 0, 255);

    return qRgb(r, g, b);
}

void Renderer::rasterize(const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2, const RenderSettings& settings) {
    int minX = std::max(0, (int)std::min({v0.screenPos.x(), v1.screenPos.x(), v2.screenPos.x()}));
    int minY = std::max(0, (int)std::min({v0.screenPos.y(), v1.screenPos.y(), v2.screenPos.y()}));
    int maxX = std::min(width - 1, (int)std::max({v0.screenPos.x(), v1.screenPos.x(), v2.screenPos.x()}) + 1);
    int maxY = std::min(height - 1, (int)std::max({v0.screenPos.y(), v1.screenPos.y(), v2.screenPos.y()}) + 1);

    float area = (v1.screenPos.y() - v2.screenPos.y()) * (v0.screenPos.x() - v2.screenPos.x()) +
                 (v2.screenPos.x() - v1.screenPos.x()) * (v0.screenPos.y() - v2.screenPos.y());

    if (std::abs(area) < 1e-5) return;
    float invArea = 1.0f / area;

    for (int y = minY; y <= maxY; y++) {
        uint32_t* scanline = (uint32_t*)frameBuffer.scanLine(y);

        for (int x = minX; x <= maxX; x++) {
            // Barycentric coordinates
            float w0 = ((v1.screenPos.y() - v2.screenPos.y()) * (x - v2.screenPos.x()) +
                        (v2.screenPos.x() - v1.screenPos.x()) * (y - v2.screenPos.y())) * invArea;
            float w1 = ((v2.screenPos.y() - v0.screenPos.y()) * (x - v2.screenPos.x()) +
                        (v0.screenPos.x() - v2.screenPos.x()) * (y - v2.screenPos.y())) * invArea;
            float w2 = 1.0f - w0 - w1;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                float z = w0 * v0.screenPos.z() + w1 * v1.screenPos.z() + w2 * v2.screenPos.z();

                if (settings.isGhost) {
                    //z -= 0.1f;
                    if ((x % 2 == 0) && (y % 2 == 0)) {
                        if (zBufferTest(x, y, z)) {
                            scanline[x] = settings.color;
                        }
                    }
                }
                else {
                    if (zBufferTest(x, y, z)) {
                        updateZBuffer(x, y, z);

                        if (settings.isShadow) {
                            scanline[x] = qRgb(30, 30, 30);
                        } else {
                            QVector3D interpNormal = v0.normal * w0 + v1.normal * w1 + v2.normal * w2;
                            QVector3D interpPos = v0.worldPos * w0 + v1.worldPos * w1 + v2.worldPos * w2;

                            scanline[x] = calculatePhongPixel(interpPos, interpNormal, settings);
                        }
                    }
                }
            }
        }
    }
}
