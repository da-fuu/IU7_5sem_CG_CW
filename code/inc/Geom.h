#ifndef GEOM_H
#define GEOM_H

#include <QVector3D>
#include <QVector4D>
#include <QMatrix4x4>
#include <QColor>
#include <vector>

struct Vertex {
    QVector3D pos;
    QVector3D normal;
    Vertex() {}
    Vertex(const QVector3D& p, const QVector3D& n) : pos(p), normal(n) {}
};

struct Triangle {
    Vertex v[3];
    QRgb color;

    Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, QRgb c) : color(c) {
        v[0] = v0; v[1] = v1; v[2] = v2;
    }
};

#endif // GEOM_H
