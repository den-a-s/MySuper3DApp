#pragma once
#include <directxmath.h>

enum class CameraMode { FPS, Orbit, Fixed };
enum class ProjMode { Persp, Ortho };

class Camera {
public:
    Camera() = default;

    void init(float aspect);
    void initFPS(const DirectX::XMFLOAT3& position, float yaw, float pitch, float aspect);
    void initOrtho(float halfWidth, float halfHeight, float aspect);
    void initOrbit(float aspect, float phi = 0.3f, float radius = 25.0f);

    void update();

    void setMode(CameraMode mode);
    CameraMode getMode() const { return mMode; }
    void setProjMode(ProjMode mode);
    ProjMode getProjMode() const { return mProjMode; }
    void setFov(float degrees);
    float getFov() const { return mFovDegrees; }

    void rotateFPS(float dYaw, float dPitch);
    void moveFPS(float forward, float right, float up);
    void moveInPlane(float forward, float right);

    void rotateOrbit(float dTheta, float dPhi);
    void zoomOrbit(float delta);
    void zoomOrtho(float delta);

    void setAspect(float aspect);
    void setTarget(const DirectX::XMFLOAT3& target) { mTarget = target; }

    DirectX::XMMATRIX getView() const { return mView; }
    DirectX::XMMATRIX getProjection() const { return mProjection; }

    float getRadius() const { return mRadius; }
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

    ProjMode mProjMode = ProjMode::Persp;
    float mFovDegrees = 60.0f;
    float mFovAngleY = DirectX::XMConvertToRadians(mFovDegrees);
    float mNearZ = 0.1f;
    float mFarZ = 4000.0f;
    float mAspect = 1.0f;
    float mOrthoHalfWidth = 5.0f;
    float mOrthoHalfHeight = 4.0f;
};
