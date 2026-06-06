#include "Camera.h"

void Camera::init(float aspect) {
    mAspect = aspect;
    recalculateProjection();
    update();
}

void Camera::update() {
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
    mProjMode = mode;
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

void Camera::rotateOrbit(float dTheta, float dPhi) {
    mTheta += dTheta;
    mPhi += dPhi;
    if (mPhi < 0.01f) mPhi = 0.01f;
    if (mPhi > DirectX::XM_PI - 0.01f) mPhi = DirectX::XM_PI - 0.01f;
}

void Camera::zoomOrbit(float delta) {
    mRadius += delta;
    if (mRadius < 2.0f) mRadius = 2.0f;
    if (mRadius > 80.0f) mRadius = 80.0f;
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
    case ProjMode::Ortho:
        mProjection = DirectX::XMMatrixOrthographicOffCenterLH(
            -15.0f * mAspect, 15.0f * mAspect, -15.0f, 15.0f, mNearZ, mFarZ);
        break;
    }
}
