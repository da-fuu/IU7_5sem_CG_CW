#include "Gear.h"
#include <QtMath>

Gear::Gear(int _q, int _r, float _rad, int _teeth, bool _driver)
    : q(_q), r(_r), radius(_rad), teeth(_teeth), angle(0), speed(0), isJam(false), isDriver(_driver)
{
    generateMesh();
    generateAxle();
}

void Gear::generateAxle() {
    axleMesh.clear();
    float r = 2.0f;
    float yBot = -12.0f;
    float yTop = -2.0f;
    int segs = 12;

    for(int i=0; i<segs; i++) {
        float a1 = i * 2 * M_PI / segs;
        float a2 = (i+1) * 2 * M_PI / segs;

        float c1 = std::cos(a1), s1 = std::sin(a1);
        float c2 = std::cos(a2), s2 = std::sin(a2);

        QVector3D p1b(c1*r, yBot, s1*r);
        QVector3D p2b(c2*r, yBot, s2*r);
        QVector3D p1t(c1*r, yTop, s1*r);
        QVector3D p2t(c2*r, yTop, s2*r);

        QVector3D n1(c1, 0, s1);
        QVector3D n2(c2, 0, s2);
        axleMesh.emplace_back(Vertex(p1b, n1), Vertex(p2b, n2), Vertex(p1t, n1), 0);
        axleMesh.emplace_back(Vertex(p1t, n1), Vertex(p2b, n2), Vertex(p2t, n2), 0);

        // Крышка
        QVector3D center(0, yTop, 0);
        QVector3D nTop(0,1,0);
        axleMesh.emplace_back(Vertex(center, nTop), Vertex(p2t, nTop), Vertex(p1t, nTop), 0);
    }
}

void Gear::generateMesh() {
    mesh.clear();
    float thickness = 5.0f;
    float toothHeight = 6.0f;
    float outerR = radius + toothHeight/2.0f;
    float innerR = radius - toothHeight/2.0f;
    float angleStep = 2 * M_PI / teeth;

    for (int i = 0; i < teeth; i++) {
        float baseAngle = i * angleStep;
        float a0 = baseAngle + 0.00f * angleStep;
        float a1 = baseAngle + 0.20f * angleStep;
        float a2 = baseAngle + 0.35f * angleStep;
        float a3 = baseAngle + 0.65f * angleStep;
        float a4 = baseAngle + 0.80f * angleStep;
        float a5 = baseAngle + 1.00f * angleStep;

        auto getPt = [&](float angle, float r, float y) {
            return QVector3D(std::cos(angle) * r, y, std::sin(angle) * r);
        };

        float yTop = thickness / 2.0f;
        float yBot = -thickness / 2.0f;

        struct PtInfo { float angle; float r; };
        std::vector<PtInfo> profile = {
            {a0, innerR}, {a1, innerR},
            {a2, outerR}, {a3, outerR},
            {a4, innerR}, {a5, innerR}
        };

        // Крышки (Плоские нормали)
        QVector3D nTop(0, 1, 0);
        QVector3D nBot(0, -1, 0);
        QVector3D centerTop(0, yTop, 0);
        QVector3D centerBot(0, yBot, 0);

        for (size_t k = 0; k < profile.size() - 1; k++) {
            QVector3D p1Top = getPt(profile[k].angle, profile[k].r, yTop);
            QVector3D p2Top = getPt(profile[k+1].angle, profile[k+1].r, yTop);
            QVector3D p1Bot = getPt(profile[k].angle, profile[k].r, yBot);
            QVector3D p2Bot = getPt(profile[k+1].angle, profile[k+1].r, yBot);

            mesh.emplace_back(Vertex(centerTop, nTop), Vertex(p2Top, nTop), Vertex(p1Top, nTop), 0);
            mesh.emplace_back(Vertex(centerBot, nBot), Vertex(p1Bot, nBot), Vertex(p2Bot, nBot), 0);

            // Бока
            QVector3D u = p2Top - p1Top;
            QVector3D v = p1Bot - p1Top;
            QVector3D faceNormal = QVector3D::crossProduct(u, v).normalized();
            mesh.emplace_back(Vertex(p1Top, faceNormal), Vertex(p2Top, faceNormal), Vertex(p1Bot, faceNormal), 0);
            mesh.emplace_back(Vertex(p1Bot, faceNormal), Vertex(p2Top, faceNormal), Vertex(p2Bot, faceNormal), 0);
        }
    }
}
