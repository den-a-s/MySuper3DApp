#pragma once
#include <DirectXMath.h>

class Camera;
class KatamariInputManager;

class KatamariBall {
public:
    KatamariBall();

    void update(float dt, const KatamariInputManager& input, const Camera& camera);
    void absorb(float objectGameSize);
    void reset();
    void resetMotion();

    float getRadius() const { return mRadius; }
    float getBaseRadius() const { return mBaseRadius; }
    const DirectX::XMFLOAT3& getPosition() const { return mPosition; }
    DirectX::XMVECTOR getOrientation() const { return mOrientation; }
    float getMoveMaxSpeed() const { return mMoveMaxSpeed; }

    void setMoveMaxSpeed(float s) { mMoveMaxSpeed = s; }
    void setPosition(const DirectX::XMFLOAT3& pos) { mPosition = pos; }
    void setBaseRadius(float r) { mBaseRadius = r; mRadius = r; }

private:
    void setDirection(const DirectX::XMVECTOR& dir);

    DirectX::XMFLOAT3 mPosition;
    float mRadius = 1.0f;
    float mBaseRadius = 1.0f;
    DirectX::XMVECTOR mOrientation;
    DirectX::XMFLOAT4 mSavedRot;
    DirectX::XMFLOAT3 mVelocity;
    float mMoveDrag = 5.0f;
    float mRotationDrag = 0.14f;
    float mRotationMaxSpeed = 0.1f;
    float mMoveMaxSpeed = 20.0f;
};
