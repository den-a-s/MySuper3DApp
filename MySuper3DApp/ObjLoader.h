#pragma once
#include <directxmath.h>
#include <vector>
#include <string>

struct VertexPNT {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texcoord;
};

struct LoadedMesh {
    std::vector<VertexPNT> vertices;
    std::vector<uint32_t> indices;
};

struct LoadedMaterial {
    std::string name;
    DirectX::XMFLOAT4 baseColor = {0.5f, 0.5f, 0.5f, 1.0f};
    DirectX::XMFLOAT4 ambientColor = {0.2f, 0.2f, 0.2f, 1.0f};
    DirectX::XMFLOAT4 specularColor = {0.0f, 0.0f, 0.0f, 1.0f};
    float specularExponent = 32.0f;
    std::string albedoPath;
    std::string normalPath;
    std::string metallicPath;
    std::string roughnessPath;
    std::string aoPath;
};

LoadedMesh parseObj(const std::string& objPath, std::vector<LoadedMaterial>& outMaterials);
LoadedMaterial parseMtl(const std::string& mtlPath, const std::string& matName);
