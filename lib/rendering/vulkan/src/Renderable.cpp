#include "Renderable.h"

#include "Mesh.h"
#include "Vertex.h"
#include "VulkanRenderer.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

namespace Bunny::Render
{
Scene::Scene(MeshAssetsBank* bank) : mMeshAssetsBank(bank)
{
}

bool Scene::loadFromGltfFile(std::string_view filePath, VulkanRenderer* renderer)
{
    std::filesystem::path path(filePath);
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
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

    //  load mesh
    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        //  temp: use index as key
        const auto id = mMeshAssetsBank->mMeshes.size();
        mMeshAssetsBank->mMeshes[id] = std::make_unique<Mesh>();
        std::unique_ptr<Mesh>& newMesh = mMeshAssetsBank->mMeshes.at(id);
        newMesh->mName = mesh.name;

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

                fastgltf::iterateAccessor<uint32_t>(gltf, indexAccessor, [&](uint32_t idx) { indices.push_back(idx); });
            }

            //  load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, posAccessor, [&vertices, initialVtx](glm::vec3 vec, size_t idx) {
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
                    [&vertices, initialVtx](glm::vec3 norm, size_t idx) { vertices[initialVtx + idx].mNormal = norm; });
            }

            //  load vertex color
            auto colorAttr = primitive.findAttribute("COLOR_0");
            if (colorAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& colorAccessor = gltf.accessors[colorAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, colorAccessor,
                    [&vertices, initialVtx](glm::vec4 col, size_t idx) { vertices[initialVtx + idx].mColor = col; });
            }

            //  load vertex texcoord
            auto TexCoordAttr = primitive.findAttribute("TEXCOORD_0");
            if (TexCoordAttr != primitive.attributes.end())
            {
                fastgltf::Accessor& texCoordAccessor = gltf.accessors[TexCoordAttr->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf, texCoordAccessor, [&vertices, initialVtx](glm::vec2 coord, size_t idx) {
                        vertices[initialVtx + idx].mTexCoord = coord;
                    });
            }

            //  load material
            //  TBD

            newMesh->mSurfaces.push_back(newSurface);
        }

        //  create mesh buffers
        renderer->createAndMapMeshBuffers(newMesh.get(), vertices, indices);
    }

    //  load nodes
    for (const fastgltf::Node& node : gltf.nodes)
    {
        const auto id = mNodes.size();
        Node& newSceneNode = mNodes[id];

        //  load node (local) transform
        std::visit(fastgltf::visitor{[&newSceneNode](fastgltf::TRS trs) {
                                         glm::vec3 tl(trs.translation[0], trs.translation[1], trs.translation[2]);
                                         glm::quat rot(
                                             trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                                         glm::vec3 sc(trs.scale[0], trs.scale[1], trs.scale[2]);

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         newSceneNode.mTransform.mMatrix = tm * rm * sm;
                                     },
                       [&newSceneNode](fastgltf::math::fmat4x4 transformMat) {
                           memcpy(&newSceneNode.mTransform.mMatrix, transformMat.data(), sizeof(transformMat));
                       }},
            node.transform);

        //  load node mesh if present
        if (node.meshIndex.has_value())
        {
            newSceneNode.mRenderComponent = std::make_unique<MeshRenderComponent>(
                mMeshAssetsBank->mMeshes.at(node.meshIndex.value()).get(), &newSceneNode);
        }
    }

    //  load node scene structure
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node = gltf.nodes[i];
        Node& sceneNode = mNodes.at(i);

        for (auto& c : node.children)
        {
            sceneNode.mChildren.push_back(&mNodes[c]);
            mNodes[c].mParent = &sceneNode;
        }
    }

    //  find root nodes, which are nodes that have no parent
    for (const auto& entry : mNodes)
    {
        if (entry.second.mParent == nullptr)
        {
            mRootNodes.push_back(&entry.second);
        }
    }

    return true;
}

void Scene::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const
{
    for (const Node* rootNode : mRootNodes)
    {
        rootNode->render(commandBuffer, parentTransformMatrix);
    }
}

MeshRenderComponent::MeshRenderComponent(const Mesh* mesh, const Node* owner) : mMesh(mesh), mOwner(owner)
{
}

void MeshRenderComponent::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const
{
    //  calculate transform matrix
    // glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), mOwner->mTransform.mPosition);
    // glm::mat4 rotationMat = glm::toMat4(mOwner->mTransform.mRotation);
    // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), mOwner->mTransform.mScale);

    glm::mat4 modelMat = parentTransformMatrix * mOwner->mTransform.mMatrix;
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

void Node::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const
{
    //  render self if render component exists
    if (mRenderComponent != nullptr)
    {
        mRenderComponent->render(commandBuffer, parentTransformMatrix);
    }

    //  render children
    glm::mat4 transformMat = parentTransformMatrix * mTransform.mMatrix;
    for (const Node* child : mChildren)
    {
        child->render(commandBuffer, transformMat);
    }
}

} // namespace Bunny::Render
