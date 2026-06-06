#include "Camera.h"

void Camera::init(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR focus, DirectX::FXMVECTOR up,
                  float orthoHalfWidth, float orthoHalfHeight, float nearZ, float farZ,
                  float aspect) {
    mView = DirectX::XMMatrixLookAtLH(eye, focus, up);
    mOrthoHalfWidth = orthoHalfWidth;
    mOrthoHalfHeight = orthoHalfHeight;
    mNearZ = nearZ;
    mFarZ = farZ;
    mAspect = aspect;
    recalculateProjection();
}

void Camera::setAspect(float aspect) {
    mAspect = aspect;
    recalculateProjection();
}

void Camera::recalculateProjection() {
    mProjection = DirectX::XMMatrixOrthographicOffCenterLH(
        -mOrthoHalfWidth * mAspect,
         mOrthoHalfWidth * mAspect,
        -mOrthoHalfHeight,
         mOrthoHalfHeight,
        mNearZ, mFarZ);
}
