#ifndef RENDERER_H
#define RENDERER_H

#include "Geom.h"
#include "Camera.h"
#include <QImage>
#include <vector>

struct RasterVertex {
    QVector3D screenPos;
    QVector3D worldPos;
    QVector3D normal;
};

struct RenderSettings {
    QRgb color;
    bool isShadow;
    bool isGhost;
};

class Renderer {
public:
    int width = 0;
    int height = 0;
    std::vector<float> zBuffer;
    QImage frameBuffer;

    QMatrix4x4 viewProjMatrix;
    QVector3D lightDir;
    QVector3D camPos;

    Renderer(int w, int h);

    void resize(int w, int h);
    void clear(QRgb color);

    void setCamera(const Camera& camera);
    bool project(const QVector3D& p, QPointF& out) const;

    void drawTriangle(const Triangle& tri, const QMatrix4x4& modelMatrix, bool isShadow = false, bool isGhost = false);

private:
    QRgb calculatePhongPixel(const QVector3D& fragPos, const QVector3D& normal, const RenderSettings& settings);

    void rasterize(const RasterVertex& v0, const RasterVertex& v1, const RasterVertex& v2, const RenderSettings& settings);

    bool zBufferTest(int x, int y, float z) const;
    void updateZBuffer(int x, int y, float z);
};

#endif // RENDERER_H
