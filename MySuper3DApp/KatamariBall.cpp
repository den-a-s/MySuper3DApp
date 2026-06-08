#include "KatamariBall.h"
#include "Camera.h"
#include "KatamariInputManager.h"

using namespace DirectX;

KatamariBall::KatamariBall() {
    mOrientation = XMQuaternionIdentity();
    XMStoreFloat4(&mSavedRot, XMQuaternionIdentity());
    mVelocity = {0, 0, 0};
}

void KatamariBall::update(float dt, const KatamariInputManager& input, const Camera& camera) {
    XMVECTOR ballPos = XMLoadFloat3(&mPosition);

    XMMATRIX invView = XMMatrixInverse(nullptr, camera.getView());
    XMVECTOR eye = invView.r[3];

    XMVECTOR toTarget = XMVectorSubtract(ballPos, eye);
    XMVECTOR forward = XMVectorSetY(toTarget, 0.0f);
    forward = XMVector3Normalize(forward);
    XMVECTOR right = XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward);

    XMVECTOR moveDir = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    if (input.isMoveForward())  moveDir = XMVectorAdd(moveDir, forward);
    if (input.isMoveBackward()) moveDir = XMVectorSubtract(moveDir, forward);
    if (input.isMoveRight())    moveDir = XMVectorAdd(moveDir, right);
    if (input.isMoveLeft())     moveDir = XMVectorSubtract(moveDir, right);

    float moveLen = XMVectorGetX(XMVector3Length(moveDir));
    if (moveLen > 0.001f) {
        setDirection(moveDir, dt);
    }

    {
        XMVECTOR s = XMLoadFloat4(&mSavedRot);
        float w = fmaxf(-1.0f, fminf(1.0f, XMVectorGetW(s)));
        float angle = 2.0f * acosf(w);
        float step = mRotationDrag * dt;
        if (angle > step && angle > 0.0001f) {
            float t = step / angle;
            s = XMQuaternionSlerp(s, XMQuaternionIdentity(), t);
        } else {
            s = XMQuaternionIdentity();
        }
        XMStoreFloat4(&mSavedRot, s);
    }

    mOrientation = XMQuaternionMultiply(mOrientation, XMLoadFloat4(&mSavedRot));
    mOrientation = XMQuaternionNormalize(mOrientation);

    float dragFactor = fmaxf(0.0f, 1.0f - mMoveDrag * dt);
    mVelocity.x *= dragFactor;
    mVelocity.z *= dragFactor;

    mPosition.x += mVelocity.x * dt;
    mPosition.z += mVelocity.z * dt;
    mPosition.y = mRadius;
}

void KatamariBall::absorb(float objectGameSize) {
    float tmp = sqrtf(mRadius * mRadius + objectGameSize * objectGameSize);
    mRadius = tmp;
    mRotationMaxSpeed = 9.0f / (tmp * tmp);
    if (mRotationMaxSpeed < 0.3f)
        mRotationMaxSpeed = 0.3f;
    mMoveMaxSpeed = 20.0f * sqrtf(tmp);
    mRotationDrag = 0.1f + 0.06f / sqrtf(tmp);
}

void KatamariBall::reset() {
    mRadius = mBaseRadius;
    XMStoreFloat4(&mSavedRot, XMQuaternionIdentity());
    mVelocity = {0, 0, 0};
    mMoveDrag = 5.0f;
    mRotationDrag = 0.14f;
    mRotationMaxSpeed = 6.0f;
    mMoveMaxSpeed = 20.0f;
}

void KatamariBall::setDirection(const XMVECTOR& dir, float dt) {
    XMVECTOR tmp = XMVectorSet(XMVectorGetX(dir), 0.0f, XMVectorGetZ(dir), 0.0f);
    tmp = XMVector3Normalize(tmp);

    XMVECTOR rollAxis = XMVector3Cross(tmp, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    float axisLen = XMVectorGetX(XMVector3Length(rollAxis));
    if (axisLen > 0.0001f) {
        rollAxis = XMVectorScale(rollAxis, 1.0f / axisLen);
        XMVECTOR q = XMQuaternionRotationAxis(rollAxis, -mRotationMaxSpeed * dt);

        XMVECTOR s = XMLoadFloat4(&mSavedRot);
        float w = fmaxf(-1.0f, fminf(1.0f, XMVectorGetW(s)));
        float angle = 2.0f * acosf(w);
        float f = fminf(1.0f, angle / 0.1f);
        XMVECTOR lerped = XMQuaternionSlerp(q, XMQuaternionIdentity(), f);
        XMVECTOR newSavedRot = XMQuaternionMultiply(s, lerped);
        newSavedRot = XMQuaternionNormalize(newSavedRot);
        XMStoreFloat4(&mSavedRot, newSavedRot);
    }

    XMStoreFloat3(&mVelocity, XMVectorScale(tmp, mMoveMaxSpeed));
}

void KatamariBall::resetMotion() {
    XMStoreFloat4(&mSavedRot, XMQuaternionIdentity());
    mVelocity = {0, 0, 0};
}
