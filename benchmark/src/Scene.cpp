#include "Scene.h"
#include <QtMath>
#include <queue>

Scene::Scene() {
    generateFloor();
}

float Scene::getGridStep() const { return hexSize * std::sqrt(3.0f); }

QVector3D Scene::hexToWorld(int q, int r) const {
    float x = hexSize * std::sqrt(3.0f) * (q + r/2.0f);
    float z = hexSize * 3.0f/2.0f * r;
    return QVector3D(x, 0, z);
}

void Scene::generateFloor() {
    floorMesh.clear();
    int r = fieldRadius;
    QRgb color = qRgb(180, 180, 180);

    for (int q = -r; q <= r; q++) {
        int r1 = std::max(-r, -q - r);
        int r2 = std::min(r, -q + r);
        for (int s = r1; s <= r2; s++) {
            QVector3D center = hexToWorld(q, s);
            center.setY(floorLevel);

            for(int i=0; i<6; i++) {
                float a1 = i * M_PI / 3.0f + M_PI/6.0f;
                float a2 = (i+1) * M_PI / 3.0f + M_PI/6.0f;
                QVector3D p1(std::cos(a1)*hexSize, 0, std::sin(a1)*hexSize);
                QVector3D p2(std::cos(a2)*hexSize, 0, std::sin(a2)*hexSize);
                QVector3D n(0,1,0);

                floorMesh.emplace_back(Vertex(center, n), Vertex(center+p2, n), Vertex(center+p1, n), color);
            }
        }
    }
}

void Scene::updateKinematics() {
    float driverSpeed = motorSpeedVal / 25.0f * motorDirection;
    for (auto& g : gears) {
        if (!g.isDriver) g.speed = 0;
        else g.speed = driverSpeed;
        g.isJam = false;
    }
    if (gears.empty()) return;

    int driverIdx = -1;
    for(size_t i=0; i<gears.size(); i++) if(gears[i].isDriver) { driverIdx = (int)i; break; }

    std::queue<int> q;
    std::vector<bool> visited(gears.size(), false);
    q.push(driverIdx);
    visited[driverIdx] = true;

    while(!q.empty()) {
        int currIdx = q.front(); q.pop();
        Gear& curr = gears[currIdx];
        if (curr.isJam) continue;

        for (size_t i = 0; i < gears.size(); i++) {
            if ((int)i == currIdx) continue;
            Gear& next = gears[i];

            QVector3D p1 = hexToWorld(curr.q, curr.r);
            QVector3D p2 = hexToWorld(next.q, next.r);
            float dist = p1.distanceToPoint(p2);

            if (std::abs(dist - (curr.radius + next.radius)) < 2.0f) {
                float ratio = curr.radius / next.radius;
                float newSpeed = -curr.speed * ratio;
                if (visited[i]) {
                    if (std::abs(next.speed - newSpeed) > 0.01f) curr.isJam = true;
                } else {
                    next.speed = newSpeed;
                    visited[i] = true;
                    float angleToNext = std::atan2(p2.z() - p1.z(), p2.x() - p1.x());
                    float desiredAngle = angleToNext * (1.0f + ratio) - curr.angle * ratio + M_PI + (M_PI / next.teeth);
                    next.angle = desiredAngle;
                    q.push((int)i);
                }
            }
        }
    }
}

void Scene::update(float dt) {
    updateKinematics();
    bool anyJam = false;
    for(const auto& g : gears) if(g.isJam) anyJam = true;
    if (!anyJam) {
        for (auto& g : gears) if (g.isDriver) g.angle += g.speed * dt;
        updateKinematics();
    }
}

int Scene::checkPlacementStatus(int q, int r, float newRadius) {
    if (std::abs(q) + std::abs(r) + std::abs(q+r) > fieldRadius * 2) return 0;
    QVector3D newPos = hexToWorld(q, r);
    bool hasConnection = false;
    for (const auto& g : gears) {
        QVector3D existingPos = hexToWorld(g.q, g.r);
        float dist = newPos.distanceToPoint(existingPos);
        if (dist < (newRadius + g.radius - 5.0f)) return 0;
        if (std::abs(dist - (newRadius + g.radius)) < 2.0f) hasConnection = true;
    }
    return hasConnection ? 1 : 2;
}

void Scene::addGear(int q, int r, float radius, int teeth) {
    for (const auto& g : gears) if(g.q == q && g.r == r) return;
    bool isDriver = gears.empty();
    gears.emplace_back(q, r, radius, teeth, isDriver);
}

void Scene::removeGear(int q, int r) {
    for (auto it = gears.begin(); it != gears.end(); ++it) {
        if (it->q == q && it->r == r) {
            if (!it->isDriver) {
                gears.erase(it);
                return;
            }
        }
    }
}
