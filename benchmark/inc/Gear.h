#ifndef GEAR_H
#define GEAR_H

#include "Geom.h"
#include <cmath>

struct Gear {
    int q, r;
    float radius;
    int teeth;
    float angle;
    float speed;
    bool isJam;
    bool isDriver;

    std::vector<Triangle> mesh;
    std::vector<Triangle> axleMesh;

    Gear(int _q, int _r, float _rad, int _teeth, bool _driver);

private:
    void generateMesh();
    void generateAxle();
};

#endif // GEAR_H
