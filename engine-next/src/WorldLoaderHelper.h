#pragma once

#include "Fundamentals.h"
#include "Vertex.h"
#include "MeshBank.h"
#include "MaterialBank.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <fastgltf/core.hpp>

#include <vector>
#include <unordered_map>

namespace Bunny::Engine
{

void addVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec4& color,
    const glm::vec2& texCoord, std::vector<uint32_t>& indices, std::vector<Render::NormalVertex>& vertices,
    std::unordered_map<Render::NormalVertex, uint32_t, Render::NormalVertex::Hash>& vertexToIndexMap);
void addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<Render::NormalVertex>& vertices,
    std::unordered_map<Render::NormalVertex, uint32_t, Render::NormalVertex::Hash>& vertexToIndexMap);
void addQuad(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<Render::NormalVertex>& vertices,
    std::unordered_map<Render::NormalVertex, uint32_t, Render::NormalVertex::Hash>& vertexToIndexMap);
const Render::IdType createCubeMeshToBank(
    Render::MeshBank<Render::NormalVertex>* meshBank, Render::IdType materialId, Render::IdType materialInstanceId);
void loadMeshFromGltf(Render::MeshBank<Render::NormalVertex>* meshBank, Render::MaterialProvider* materialBank,
    fastgltf::Asset& gltfAsset);

} // namespace Bunny::Engine
