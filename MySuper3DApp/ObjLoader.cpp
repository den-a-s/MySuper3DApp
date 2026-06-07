#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <filesystem>

struct IndexTriple {
    int vi, ti, ni;
    bool operator==(const IndexTriple& o) const {
        return vi == o.vi && ti == o.ti && ni == o.ni;
    }
};

struct IndexTripleHash {
    size_t operator()(const IndexTriple& t) const {
        return ((size_t)(uint32_t)t.vi << 0) ^
               ((size_t)(uint32_t)t.ti << 10) ^
               ((size_t)(uint32_t)t.ni << 20);
    }
};

static std::string dirName(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    return (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
}

LoadedMaterial parseMtl(const std::string& mtlPath, const std::string& matName) {
    LoadedMaterial mat;
    mat.name = matName;
    std::ifstream f(mtlPath);
    if (!f.is_open()) return mat;

    std::string line;
    bool found = false;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "newmtl") {
            std::string nm;
            iss >> nm;
            found = (nm == matName);
        }
        if (!found) continue;;

        if (token == "Ka") {
            float r, g, b; iss >> r >> g >> b;
            mat.ambientColor = {r, g, b, 1.0f};
        } else if (token == "Kd") {
            float r, g, b; iss >> r >> g >> b;
            mat.baseColor = {r, g, b, 1.0f};
        } else if (token == "Ks") {
            float r, g, b; iss >> r >> g >> b;
            mat.specularColor = {r, g, b, 1.0f};
        } else if (token == "Ns") {
            float v; iss >> v;
            mat.specularExponent = v;
        } else if (token == "map_Kd" || token == "map_Ka") {
            std::string texPath;
            std::getline(iss, texPath);
            texPath.erase(0, texPath.find_first_not_of(" \t"));
            if (!texPath.empty()) {
                auto mtlDir = dirName(mtlPath);
                mat.albedoPath = mtlDir + texPath;
            }
        } else if (token == "map_bump" || token == "bump") {
            std::string texPath;
            std::getline(iss, texPath);
            texPath.erase(0, texPath.find_first_not_of(" \t"));
            if (!texPath.empty()) {
                auto mtlDir = dirName(mtlPath);
                mat.normalPath = mtlDir + texPath;
            }
        }
    }
    return mat;
}

LoadedMesh parseObj(const std::string& objPath, std::vector<LoadedMaterial>& outMaterials) {
    LoadedMesh mesh;
    std::ifstream f(objPath);
    if (!f.is_open()) return mesh;

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> texcoords;

    positions.reserve(100000);
    normals.reserve(100000);
    texcoords.reserve(100000);

    std::string mtlLib;
    std::string currentMatName;
    auto objDir = dirName(objPath);

    std::unordered_map<IndexTriple, uint32_t, IndexTripleHash> vertCache;
    std::vector<IndexTriple> triIndices;

    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "v") {
            float x, y, z; iss >> x >> y >> z;
            positions.push_back({x, y, z});
        } else if (token == "vn") {
            float x, y, z; iss >> x >> y >> z;
            normals.push_back({x, y, z});
        } else if (token == "vt") {
            float u, v; iss >> u >> v;
            texcoords.push_back({u, v});
        } else if (token == "f") {
            std::string vertStr;
            std::vector<int> vi, ti, ni;
            while (iss >> vertStr) {
                std::replace(vertStr.begin(), vertStr.end(), '/', ' ');
                std::istringstream vis(vertStr);
                int v = 0, t = 0, n = 0;
                vis >> v;
                if (!vis.eof()) { vis >> t; if (t < 0) t = 0; }
                if (!vis.eof()) { vis >> n; if (n < 0) n = 0; }
                vi.push_back(v > 0 ? v - 1 : (int)positions.size() + v);
                ti.push_back(t > 0 ? t - 1 : -1);
                ni.push_back(n > 0 ? n - 1 : -1);
            }
            for (size_t i = 1; i + 1 < vi.size(); ++i) {
                int idxs[] = {0, (int)i, (int)i + 1};
                for (int k = 0; k < 3; ++k) {
                    int idx = idxs[k];
                    IndexTriple key = {vi[idx], ti[idx], ni[idx]};
                    auto it = vertCache.find(key);
                    if (it != vertCache.end()) {
                        mesh.indices.push_back(it->second);
                    } else {
                        VertexPNT v;
                        v.position = (key.vi >= 0 && key.vi < (int)positions.size())
                            ? positions[key.vi] : DirectX::XMFLOAT3(0,0,0);
                        v.normal = (key.ni >= 0 && key.ni < (int)normals.size())
                            ? normals[key.ni] : DirectX::XMFLOAT3(0,1,0);
                        v.texcoord = (key.ti >= 0 && key.ti < (int)texcoords.size())
                            ? texcoords[key.ti] : DirectX::XMFLOAT2(0,0);
                        uint32_t newIdx = (uint32_t)mesh.vertices.size();
                        mesh.vertices.push_back(v);
                        vertCache[key] = newIdx;
                        mesh.indices.push_back(newIdx);
                    }
                }
            }
        } else if (token == "mtllib") {
            std::string lib;
            std::getline(iss, lib);
            lib.erase(0, lib.find_first_not_of(" \t"));
            mtlLib = objDir + lib;
        } else if (token == "usemtl") {
            iss >> currentMatName;
        }
    }

    if (!mtlLib.empty() && !currentMatName.empty()) {
        auto mat = parseMtl(mtlLib, currentMatName);
        outMaterials.push_back(mat);
    }

    if (normals.empty()) {
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            auto& v0 = mesh.vertices[mesh.indices[i]];
            auto& v1 = mesh.vertices[mesh.indices[i + 1]];
            auto& v2 = mesh.vertices[mesh.indices[i + 2]];
            DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.position);
            DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.position);
            DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&v2.position);
            DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(p1, p0);
            DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(p2, p0);
            DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(edge1, edge2));
            DirectX::XMFLOAT3 fn;
            DirectX::XMStoreFloat3(&fn, n);
            v0.normal = fn; v1.normal = fn; v2.normal = fn;
        }
    }

    return mesh;
}
