#pragma once
#include <directxmath.h>

class Camera {
public:
    Camera() = default;

    void init(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR focus, DirectX::FXMVECTOR up,
              float orthoHalfWidth, float orthoHalfHeight, float nearZ, float farZ,
              float aspect);

    void setAspect(float aspect);

    DirectX::XMMATRIX getView() const { return mView; }
    DirectX::XMMATRIX getProjection() const { return mProjection; }

private:
    void recalculateProjection();

    DirectX::XMMATRIX mView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX mProjection = DirectX::XMMatrixIdentity();
    float mOrthoHalfWidth = 5.0f;
    float mOrthoHalfHeight = 4.0f;
    float mNearZ = 0.1f;
    float mFarZ = 100.0f;
    float mAspect = 1.0f;
};
