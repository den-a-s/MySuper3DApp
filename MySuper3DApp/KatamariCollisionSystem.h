#pragma once
#include <DirectXMath.h>
#include <functional>
#include <vector>
#include <string>

struct KatamariGameObject;
class KatamariBall;

DirectX::XMVECTOR computeObjectWorldPos(
    const std::vector<KatamariGameObject>& objects,
    int index,
    int ballIndex,
    DirectX::XMVECTOR ballPos,
    DirectX::XMVECTOR ballOrientation);

void katamariCheckPickups(
    std::vector<KatamariGameObject>& objects,
    int ballIndex,
    KatamariBall& ball,
    const std::function<void(const std::string&)>& logCallback);
