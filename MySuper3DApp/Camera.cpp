#include "Camera.h"

void Camera::init(float aspect) {
    mMode = CameraMode::FPS;
    mPosition = {0.0f, 5.0f, -15.0f};
    mYaw = 0.0f;
    mPitch = 0.3f;
    mTheta = 0.0f;
    mPhi = DirectX::XM_PIDIV4;
    mRadius = 25.0f;
    mTarget = {0.0f, 0.0f, 0.0f};
    mProjMode = ProjMode::Persp;
    mFovDegrees = 60.0f;
    mFovAngleY = DirectX::XMConvertToRadians(mFovDegrees);
    mAspect = aspect;
    recalculateProjection();
    update();
}

void Camera::initOrbit(float aspect, float phi, float radius) {
    mMode = CameraMode::Orbit;
    mTheta = 0.0f;
    mPhi = phi;
    mRadius = radius;
    mTarget = {0.0f, 0.0f, 0.0f};
    mProjMode = ProjMode::Persp;
    mFovDegrees = 60.0f;
    mFovAngleY = DirectX::XMConvertToRadians(mFovDegrees);
    mAspect = aspect;
    recalculateProjection();
    update();
}

void Camera::initFPS(const DirectX::XMFLOAT3& position, float yaw, float pitch, float aspect) {
    mMode = CameraMode::FPS;
    mPosition = position;
    mYaw = yaw;
    mPitch = pitch;
    mProjMode = ProjMode::Persp;
    mFovDegrees = 60.0f;
    mFovAngleY = DirectX::XMConvertToRadians(mFovDegrees);
    mAspect = aspect;
    recalculateProjection();
    update();
}

void Camera::initOrtho(float halfWidth, float halfHeight, float aspect) {
    mOrthoHalfHeight = halfHeight;
    mMode = CameraMode::Fixed;
    mPosition = {0.0f, 0.0f, -10.0f};
    DirectX::XMVECTOR eye = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f);
    DirectX::XMVECTOR focus = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    mView = DirectX::XMMatrixLookAtLH(eye, focus, up);
    mNearZ = 0.1f;
    mFarZ = 4000.0f;
    mAspect = aspect;
    mProjMode = ProjMode::Ortho;
    recalculateProjection();
}

void Camera::update() {
    if (mMode == CameraMode::Fixed) return;
    if (mMode == CameraMode::FPS) {
        float cosPitch = cosf(mPitch);
        float sinPitch = sinf(mPitch);
        float cosYaw = cosf(mYaw);
        float sinYaw = sinf(mYaw);

        DirectX::XMVECTOR forward = DirectX::XMVectorSet(
            cosPitch * sinYaw, sinPitch, cosPitch * cosYaw, 0.0f);

        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&mPosition);
        DirectX::XMVECTOR focus = DirectX::XMVectorAdd(pos, forward);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        mView = DirectX::XMMatrixLookAtLH(pos, focus, up);
    } else {
        float x = mTarget.x + mRadius * sinf(mPhi) * cosf(mTheta);
        float y = mTarget.y + mRadius * cosf(mPhi);
        float z = mTarget.z + mRadius * sinf(mPhi) * sinf(mTheta);

        DirectX::XMVECTOR eye = DirectX::XMVectorSet(x, y, z, 0.0f);
        DirectX::XMVECTOR focus = DirectX::XMVectorSet(mTarget.x, mTarget.y, mTarget.z, 0.0f);
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        mView = DirectX::XMMatrixLookAtLH(eye, focus, up);
    }
}

void Camera::setMode(CameraMode mode) {
    mMode = mode;
}

void Camera::setProjMode(ProjMode mode) {
    if (mode == mProjMode) return;
    mProjMode = mode;
    if (mode == ProjMode::Ortho) {
        float dist;
        if (mMode == CameraMode::Orbit) {
            dist = mRadius;
        } else {
            dist = sqrtf(mPosition.x * mPosition.x + mPosition.y * mPosition.y + mPosition.z * mPosition.z);
        }
        mOrthoHalfHeight = dist * tanf(DirectX::XMConvertToRadians(mFovDegrees * 0.5f));
    }
    recalculateProjection();
}

void Camera::setFov(float degrees) {
    mFovDegrees = degrees;
    mFovAngleY = DirectX::XMConvertToRadians(degrees);
    if (mProjMode == ProjMode::Persp)
        recalculateProjection();
}

void Camera::rotateFPS(float dYaw, float dPitch) {
    mYaw += dYaw;
    mPitch += dPitch;
    if (mPitch > DirectX::XM_PIDIV2 - 0.01f) mPitch = DirectX::XM_PIDIV2 - 0.01f;
    if (mPitch < -DirectX::XM_PIDIV2 + 0.01f) mPitch = -DirectX::XM_PIDIV2 + 0.01f;
}

void Camera::moveFPS(float forward, float right, float up) {
    float cosYaw = cosf(mYaw);
    float sinYaw = sinf(mYaw);

    DirectX::XMVECTOR fwd = DirectX::XMVectorSet(
        cosf(mPitch) * sinYaw, sinf(mPitch), cosf(mPitch) * cosYaw, 0.0f);
    DirectX::XMVECTOR rgt = DirectX::XMVectorSet(cosYaw, 0.0f, -sinYaw, 0.0f);
    DirectX::XMVECTOR upv = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&mPosition);
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(fwd, forward));
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(rgt, right));
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(upv, up));
    DirectX::XMStoreFloat3(&mPosition, pos);
}

void Camera::moveInPlane(float forward, float right) {
    float cosYaw = cosf(mYaw);
    float sinYaw = sinf(mYaw);
    DirectX::XMVECTOR fwd = DirectX::XMVectorSet(sinYaw, 0.0f, cosYaw, 0.0f);
    DirectX::XMVECTOR rgt = DirectX::XMVectorSet(cosYaw, 0.0f, -sinYaw, 0.0f);
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&mPosition);
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(fwd, forward));
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(rgt, right));
    DirectX::XMStoreFloat3(&mPosition, pos);
}

void Camera::rotateOrbit(float dTheta, float dPhi) {
    mTheta += dTheta;
    mPhi += dPhi;
    if (mPhi < 0.01f) mPhi = 0.01f;
    if (mPhi > DirectX::XM_PI - 0.01f) mPhi = DirectX::XM_PI - 0.01f;
}

void Camera::zoomOrbit(float delta) {
    mRadius += delta;
    if (mRadius < 2.0f) mRadius = 2.0f;
    if (mRadius > 200.0f) mRadius = 200.0f;
}

void Camera::zoomOrtho(float delta) {
    mOrthoHalfHeight += delta;
    if (mOrthoHalfHeight < 0.1f) mOrthoHalfHeight = 0.1f;
    recalculateProjection();
}

void Camera::setAspect(float aspect) {
    mAspect = aspect;
    recalculateProjection();
}

void Camera::recalculateProjection() {
    switch (mProjMode) {
    case ProjMode::Persp:
        mFovAngleY = DirectX::XMConvertToRadians(mFovDegrees);
        mProjection = DirectX::XMMatrixPerspectiveFovLH(mFovAngleY, mAspect, mNearZ, mFarZ);
        break;
    case ProjMode::Ortho: {
        float halfW = mOrthoHalfHeight * mAspect;
        mProjection = DirectX::XMMatrixOrthographicOffCenterLH(
            -halfW, halfW,
            -mOrthoHalfHeight, mOrthoHalfHeight,
            mNearZ, mFarZ);
        break;
    }
    }
}
