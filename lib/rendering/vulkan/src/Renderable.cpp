#include "Renderable.h"

#include "Mesh.h"
#include "Vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

namespace Bunny::Render
{
Scene::Scene(MeshAssetsBank* bank)
    : mMeshAssetsBank(bank)
{
}

bool Scene::loadFromGltfFile(std::string_view filePath)
{
    std::filesystem::path path(filePath);
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;
    auto loadedFile = fastgltf::GltfDataBuffer::FromPath(filePath);
    if (loadedFile.error() != fastgltf::Error::None)
    {
        return false;
    }
    fastgltf::Parser parser;
    auto parseResult = parser.loadGltf(loadedFile.get(), path.parent_path(), gltfOptions);
    if (parseResult.error() != fastgltf::Error::None)
    {
        return false;
    }
    fastgltf::Asset& gltf = parseResult.get();

    std::vector<uint32_t> indices;
    std::vector<NormalVertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        Mesh newMesh;
        newMesh.mName = mesh.name;

        indices.clear();
        vertices.clear();

        //  create mesh surfaces
        for (auto&& primitive : mesh.primitives)
        {
            Surface newSurface;
            newSurface.mStartIndex = indices.size();

            size_t initialVtx = vertices.size();

            //  load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
                newSurface.mCount = indexAccessor.count;

                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<uint32_t>(gltf, indexAccessor, [&](uint32_t idx){
                    indices.push_back(idx);
                });
            }

            //  load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                [&vertices, initialVtx](glm::vec3 vec, size_t idx){
                    NormalVertex newVertex;
                    newVertex.mPosition = vec;
                    newVertex.mNormal = {1, 0, 0};
                    newVertex.mColor = {0.8f, 0.8f, 0.8f, 1.0f};
                    newVertex.mTexCoord = {0, 0};
                    vertices[initialVtx + idx] = newVertex;
                });
            }

            //  load vertex normal
            auto normalAttr = primitive.findAttribute("NORMAL");
            if (normalAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& normalAccessor = gltf.accessors[normalAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, normalAccessor,
                [&vertices, initialVtx](glm::vec3 norm, size_t idx) {
                    vertices[initialVtx + idx].mNormal = norm;
                });
            }

            //  load vertex color
            auto colorAttr = primitive.findAttribute("COLOR_0");
            if (colorAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& colorAccessor = gltf.accessors[colorAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colorAccessor,
                [&vertices, initialVtx](glm::vec4 col, size_t idx) {
                    vertices[initialVtx + idx].mColor = col;
                });
            }

            //  load vertex texcoord
            auto TexCoordAttr = primitive.findAttribute("TEXCOORD_0");
            if (TexCoordAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& texCoordAccessor = gltf.accessors[TexCoordAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, texCoordAccessor,
                [&vertices, initialVtx](glm::vec2 coord, size_t idx) {
                    vertices[initialVtx + idx].mTexCoord = coord;
                });
            }
        
            //  load material
            //  TBD

            newMesh.mSurfaces.push_back(newSurface);
        }
    }

    return true;
}

void Scene::render(VkCommandBuffer commandBuffer)
{
}

MeshRenderComponent::MeshRenderComponent(const Mesh* mesh) : mMesh(mesh)
{
}

void MeshRenderComponent::render(VkCommandBuffer commandBuffer)
{
    //  calculate transform matrix
    glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), mTransform.mPosition);
    glm::mat4 rotationMat = glm::toMat4(mTransform.mRotation);
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), mTransform.mScale);

    glm::mat4 modelMat = translateMat * rotationMat * scaleMat;
    glm::mat4 invTransMat = glm::inverse(glm::transpose(modelMat));

    //  bind buffers
    VkBuffer vertexBuffers[] = {mMesh->mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, mMesh->mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    RenderPushConstant pushConst{.model = modelMat, .invTransModel = invTransMat};

    //  draw!!
    for (const auto& surface : mMesh->mSurfaces)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, surface.mMaterial.mpBaseMaterial->mPipiline);
        vkCmdPushConstants(commandBuffer, surface.mMaterial.mpBaseMaterial->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(RenderPushConstant), &pushConst);
        vkCmdDrawIndexed(commandBuffer, surface.mCount, 1, surface.mStartIndex, 0, 0);
    }
}

} // namespace Bunny::Render
