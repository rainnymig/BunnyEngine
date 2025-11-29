#pragma once

#include "Fundamentals.h"
#include "Vertex.h"
#include "MeshBank.h"
#include "MaterialBank.h"
#include "TextureBank.h"
#include "Error.h"

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
void loadMeshFromGltf(Render::MeshBank<Render::NormalVertex>* meshBank, Render::PbrMaterialBank* materialBank,
    Render::TextureBank* textureBank, fastgltf::Asset& gltfAsset);

template <typename TextureInfoType>
Bunny::Render::IdType loadTextureFromGltf(const fastgltf::Optional<TextureInfoType>& gltfTexInfo,
    const fastgltf::Asset& gltfAsset, Render::TextureBank* textureBank)
{
    Render::IdType newId = Render::BUNNY_INVALID_ID;

    if (gltfTexInfo.has_value())
    {
        size_t texId = gltfTexInfo.value().textureIndex;
        const auto imageIdx = gltfAsset.textures[texId].imageIndex;
        if (imageIdx.has_value())
        {
            const auto& imageAsset = gltfAsset.images[imageIdx.value()];
            //  now we only deal with textures stored directly in gltf buffer in the asset
            //  deal with texture file variant later
            if (const fastgltf::sources::BufferView* bufferViewPtr =
                    std::get_if<fastgltf::sources::BufferView>(&imageAsset.data))
            {
                const fastgltf::BufferView& imgBufView = gltfAsset.bufferViews[bufferViewPtr->bufferViewIndex];
                const fastgltf::Buffer& imgBuf = gltfAsset.buffers[imgBufView.bufferIndex];
                if (const fastgltf::sources::Array* bufDataArray = std::get_if<fastgltf::sources::Array>(&imgBuf.data))
                {
                    unsigned char* imageData = (unsigned char*)bufDataArray->bytes.data();
                    Render::IdType newTexId;
                    if (BUNNY_SUCCESS(textureBank->addTextureFromMemory(imageData + imgBufView.byteOffset,
                            imgBufView.byteLength, VK_FORMAT_R8G8B8A8_UNORM, newTexId)))
                    {
                        newId = newTexId;
                    }
                }
            }
        }
    }

    return newId;
}

} // namespace Bunny::Engine
