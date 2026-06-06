#pragma once
#include <directxmath.h>

class Camera {
public:
    Camera() = default;

    void init(float aspect);

    void update();
    void rotate(float dYaw, float dPitch);
    void move(float forward, float right, float up);

    void setAspect(float aspect);

    DirectX::XMMATRIX getView() const { return mView; }
    DirectX::XMMATRIX getProjection() const { return mProjection; }

    DirectX::XMFLOAT3 getPosition() const { return mPosition; }

private:
    void recalculateProjection();

    DirectX::XMMATRIX mView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX mProjection = DirectX::XMMatrixIdentity();

    DirectX::XMFLOAT3 mPosition = {0.0f, 5.0f, -15.0f};
    float mYaw = 0.0f;
    float mPitch = 0.3f;

    float mFovAngleY = DirectX::XMConvertToRadians(60.0f);
    float mNearZ = 0.1f;
    float mFarZ = 200.0f;
    float mAspect = 1.0f;
};
