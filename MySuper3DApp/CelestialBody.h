#pragma once
#include <directxmath.h>

struct MoonData {
    float orbitRadius;
    float orbitSpeed;
    float angle;
    float rotationSpeed;
    float rotationAngle;
    float size;
    DirectX::XMFLOAT4 color;
};

struct PlanetData {
    float orbitRadius;
    float orbitSpeed;
    float angle;
    float rotationSpeed;
    float rotationAngle;
    float size;
    DirectX::XMFLOAT4 color;
    MoonData moon;
};
