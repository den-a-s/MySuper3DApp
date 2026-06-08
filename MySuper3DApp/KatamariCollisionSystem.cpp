#include "KatamariCollisionSystem.h"
#include "KatamariBall.h"
#include "KatamariAtHomeState.h"

using namespace DirectX;

XMVECTOR computeObjectWorldPos(
    const std::vector<KatamariGameObject>& objects,
    int index,
    int ballIndex,
    XMVECTOR ballPos,
    XMVECTOR ballOrientation)
{
    if (index == ballIndex)
        return ballPos;

    auto& obj = objects[index];
    XMVECTOR localOffset = XMLoadFloat3(&obj.attachOffset);
    XMVECTOR worldOffset = XMVector3Rotate(localOffset, ballOrientation);

    if (obj.parentIndex == -2) {
        return XMVectorAdd(ballPos, worldOffset);
    } else if (obj.parentIndex >= 0) {
        XMVECTOR parentPos = computeObjectWorldPos(objects, obj.parentIndex, ballIndex, ballPos, ballOrientation);
        return XMVectorAdd(parentPos, worldOffset);
    }
    return ballPos;
}

XMVECTOR computeObjectWorldRot(
    const std::vector<KatamariGameObject>& objects,
    int index, int ballIndex,
    XMVECTOR ballOrientation)
{
    if (index == ballIndex)
        return ballOrientation;

    auto& obj = objects[index];
    XMVECTOR localRot = XMLoadFloat4(&obj.attachRotation);

    if (obj.parentIndex == -2) {
        return XMQuaternionMultiply(localRot, ballOrientation);
    } else if (obj.parentIndex >= 0) {
        XMVECTOR parentRot = computeObjectWorldRot(objects, obj.parentIndex, ballIndex, ballOrientation);
        return XMQuaternionMultiply(localRot, parentRot);
    }
    return ballOrientation;
}

void katamariCheckPickups(
    std::vector<KatamariGameObject>& objects,
    int ballIndex,
    KatamariBall& ball,
    const std::function<void(const std::string&)>& logCallback)
{
    if (ballIndex < 0) return;
    XMVECTOR ballPos = XMLoadFloat3(&ball.getPosition());
    float ballRadius = ball.getRadius();
    XMVECTOR ballOrientation = ball.getOrientation();

    for (int i = 0; i < (int)objects.size(); ++i) {
        if (i == ballIndex || objects[i].isAttached) continue;
        auto& obj = objects[i];

        XMVECTOR objPos = XMLoadFloat3(&obj.position);
        XMVECTOR diff = XMVectorSubtract(objPos, ballPos);
        float dist = XMVectorGetX(XMVector3Length(diff));
        float objR = obj.boundingRadius * obj.scale * obj.scaleMultiplier;
        float sumRadii = ballRadius + objR;

        if (dist < sumRadii && ballRadius > obj.gameSize) {
            XMVECTOR dir = XMVector3Normalize(diff);
            if (XMVectorGetX(XMVector3Length(diff)) < 0.0001f) {
                dir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
            }
            XMVECTOR worldOffset = XMVectorScale(dir, ballRadius + objR * 0.7f);
            XMVECTOR invOrient = XMQuaternionInverse(ballOrientation);
            XMVECTOR localOffset = XMVector3Rotate(worldOffset, invOrient);
            XMStoreFloat3(&obj.attachOffset, localOffset);
            {
                XMVECTOR objWorldRot = XMQuaternionRotationRollPitchYaw(
                    obj.rotation.x, obj.rotation.y, obj.rotation.z);
                XMVECTOR localRot = XMQuaternionMultiply(objWorldRot, invOrient);
                localRot = XMQuaternionNormalize(localRot);
                XMStoreFloat4(&obj.attachRotation, localRot);
            }
            obj.isAttached = true;
            obj.parentIndex = -2;
            ball.absorb(obj.gameSize);
            logCallback("Picked up " + obj.name);
        }
    }

    for (int i = 0; i < (int)objects.size(); ++i) {
        if (i == ballIndex || objects[i].isAttached) continue;
        auto& freeObj = objects[i];
        XMVECTOR freePos = XMLoadFloat3(&freeObj.position);

        for (int j = 0; j < (int)objects.size(); ++j) {
            if (j == ballIndex || !objects[j].isAttached) continue;

            XMVECTOR attachedPos = computeObjectWorldPos(objects, j, ballIndex, ballPos, ballOrientation);
            XMVECTOR diff = XMVectorSubtract(freePos, attachedPos);
            float dist = XMVectorGetX(XMVector3Length(diff));
            float freeR = freeObj.boundingRadius * freeObj.scale * freeObj.scaleMultiplier;
            float attachedR = objects[j].boundingRadius * objects[j].scale * objects[j].scaleMultiplier;

            if (dist < freeR + attachedR && ballRadius > freeObj.gameSize) {
                XMVECTOR dir = XMVector3Normalize(diff);
                if (XMVectorGetX(XMVector3Length(diff)) < 0.0001f) {
                    dir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
                }
                XMVECTOR worldOffset = XMVectorScale(dir, attachedR + freeR * 0.7f);
                XMVECTOR invOrient = XMQuaternionInverse(ballOrientation);
                XMVECTOR localOffset = XMVector3Rotate(worldOffset, invOrient);
                XMStoreFloat3(&freeObj.attachOffset, localOffset);
                {
                    XMVECTOR objWorldRot = XMQuaternionRotationRollPitchYaw(
                        freeObj.rotation.x, freeObj.rotation.y, freeObj.rotation.z);
                    XMVECTOR parentWorldRot = computeObjectWorldRot(objects, j, ballIndex, ballOrientation);
                    XMVECTOR invParentRot = XMQuaternionInverse(parentWorldRot);
                    XMVECTOR localRot = XMQuaternionMultiply(objWorldRot, invParentRot);
                    localRot = XMQuaternionNormalize(localRot);
                    XMStoreFloat4(&freeObj.attachRotation, localRot);
                }
                freeObj.isAttached = true;
                freeObj.parentIndex = j;
                ball.absorb(freeObj.gameSize);
                logCallback("Picked up " + freeObj.name + " (chain)");
                break;
            }
        }
    }
}
