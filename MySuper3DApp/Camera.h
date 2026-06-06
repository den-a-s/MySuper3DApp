#pragma once
#include <directxmath.h>

enum class CameraMode { FPS, Orbit };
enum class ProjMode { Persp60, Persp90, Persp30, Ortho };

class Camera {
public:
    Camera() = default;

    void init(float aspect);

    void update();

    void setMode(CameraMode mode);
    CameraMode getMode() const { return mMode; }
    void cycleProj();
    int getProjIndex() const { return static_cast<int>(mProjMode); }

    void rotateFPS(float dYaw, float dPitch);
    void moveFPS(float forward, float right, float up);

    void rotateOrbit(float dTheta, float dPhi);
    void zoomOrbit(float delta);

    void setAspect(float aspect);

    DirectX::XMMATRIX getView() const { return mView; }
    DirectX::XMMATRIX getProjection() const { return mProjection; }

    CameraMode mMode = CameraMode::FPS;

private:
    void recalculateProjection();

    DirectX::XMMATRIX mView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX mProjection = DirectX::XMMatrixIdentity();

    DirectX::XMFLOAT3 mPosition = {0.0f, 5.0f, -15.0f};
    float mYaw = 0.0f;
    float mPitch = 0.3f;

    float mTheta = 0.0f;
    float mPhi = DirectX::XM_PIDIV4;
    float mRadius = 25.0f;
    DirectX::XMFLOAT3 mTarget = {0.0f, 0.0f, 0.0f};

    ProjMode mProjMode = ProjMode::Persp60;
    float mFovAngleY = DirectX::XMConvertToRadians(60.0f);
    float mNearZ = 0.1f;
    float mFarZ = 200.0f;
    float mAspect = 1.0f;
};
