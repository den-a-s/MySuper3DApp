#include "Camera.h"

void Camera::init(float aspect) {
    mAspect = aspect;
    recalculateProjection();
    update();
}

void Camera::update() {
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
}

void Camera::rotate(float dYaw, float dPitch) {
    mYaw += dYaw;
    mPitch += dPitch;
    if (mPitch > DirectX::XM_PIDIV2 - 0.01f) mPitch = DirectX::XM_PIDIV2 - 0.01f;
    if (mPitch < -DirectX::XM_PIDIV2 + 0.01f) mPitch = -DirectX::XM_PIDIV2 + 0.01f;
}

void Camera::move(float forward, float right, float up) {
    float cosPitch = cosf(mPitch);
    float sinPitch = sinf(mPitch);
    float cosYaw = cosf(mYaw);
    float sinYaw = sinf(mYaw);

    DirectX::XMVECTOR fwd = DirectX::XMVectorSet(
        cosPitch * sinYaw, sinPitch, cosPitch * cosYaw, 0.0f);
    DirectX::XMVECTOR rgt = DirectX::XMVectorSet(
        cosYaw, 0.0f, -sinYaw, 0.0f);
    DirectX::XMVECTOR upv = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&mPosition);
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(fwd, forward));
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(rgt, right));
    pos = DirectX::XMVectorAdd(pos, DirectX::XMVectorScale(upv, up));
    DirectX::XMStoreFloat3(&mPosition, pos);
}

void Camera::setAspect(float aspect) {
    mAspect = aspect;
    recalculateProjection();
}

void Camera::recalculateProjection() {
    mProjection = DirectX::XMMatrixPerspectiveFovLH(mFovAngleY, mAspect, mNearZ, mFarZ);
}
