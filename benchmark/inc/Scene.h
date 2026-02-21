#ifndef SCENE_H
#define SCENE_H

#include "Gear.h"
#include <vector>

class Scene {
public:
    std::vector<Gear> gears;
    std::vector<Triangle> floorMesh;

    float motorSpeedVal = 10.0f;
    int motorDirection = 1;

    const float hexSize = 20.0f;
    const int fieldRadius = 15;
    const float floorLevel = 0.0f;
    const float gearLevel = 12.0f;

    Scene();

    float getGridStep() const;
    QVector3D hexToWorld(int q, int r) const;

    int checkPlacementStatus(int q, int r, float newRadius);
    void addGear(int q, int r, float radius, int teeth);
    void removeGear(int q, int r);
    void update(float dt);

private:
    void generateFloor();
    void updateKinematics();
};

#endif // SCENE_H
