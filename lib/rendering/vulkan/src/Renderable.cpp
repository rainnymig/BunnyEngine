#include "Renderable.h"

#include "Mesh.h"
#include "Vertex.h"
#include "VulkanRenderer.h"
#include "Transform.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <chrono>

namespace Bunny::Render
{
void Scene::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix)
{
    //  handle camera data
    //  handle lights data
    //  no change

    // clear desc pools to free all previous descriptor sets
    mDescriptorAllocators[mRenderer->getCurrentFrameId()].clearPools(mDevice);

    // VkDescriptorSetLayout descLayouts[] = {mSceneDescSetLayout, mObjectDescSetLayout};
    // std::array<VkDescriptorSet, 2> descSets{mSceneDescriptorSets[mRenderer->getCurrentFrameId()],
    //     nullptr}; //  scene (including scene data and light data) and objects
    mDescriptorAllocators[mRenderer->getCurrentFrameId()].allocate(
        mDevice, &mSceneDescSetLayout, &mSceneDescriptorSets[mRenderer->getCurrentFrameId()], 1, nullptr);

    {
        DescriptorWriter writer;
        writer.writeBuffer(0, mSceneDataBuffer.mBuffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.writeBuffer(1, mLightDataBuffer.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.updateSet(mDevice, mSceneDescriptorSets[mRenderer->getCurrentFrameId()]);
    }

    for (Node* rootNode : mRootNodes)
    {
        rootNode->render(commandBuffer, parentTransformMatrix);
    }
}

void Scene::update()
{
    // static auto startTime = std::chrono::high_resolution_clock::now();
    // auto currentTime = std::chrono::high_resolution_clock::now();
    // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
}

void Scene::destroyScene()
{
    mRenderer->destroyBuffer(mSceneDataBuffer);
    mRenderer->destroyBuffer(mLightDataBuffer);
    mRenderer->destroyBuffer(mObjectDataBuffer);
    for (DescriptorAllocator& alloc : mDescriptorAllocators)
    {
        alloc.destroyPools(mDevice);
    }
    vkDestroyDescriptorSetLayout(mDevice, mSceneDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mObjectDescSetLayout, nullptr);
}

void Scene::findRootNodes()
{
    mRootNodes.clear();

    for (auto& entry : mNodes)
    {
        if (entry.second.mParent == nullptr)
        {
            mRootNodes.push_back(&entry.second);
        }
    }
}

void Scene::buildDescriptorSetLayout()
{
    DescriptorLayoutBuilder builder;

    //  scene set
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.addBinding(uniformBufferLayout);
    }
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 1;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.addBinding(uniformBufferLayout);
    }
    mSceneDescSetLayout = builder.build(mDevice);

    builder.clear();

    //  object set
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.addBinding(uniformBufferLayout);
    }
    mObjectDescSetLayout = builder.build(mDevice);
}

void Scene::initDescriptorAllocator()
{
    constexpr size_t MAX_SETS = 6;
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 3},
    };
    for (DescriptorAllocator& alloc : mDescriptorAllocators)
    {
        alloc.init(mDevice, MAX_SETS, poolSizes);
    }
}

void Scene::initBuffers()
{
    //  create buffers
    mSceneDataBuffer = mRenderer->createBuffer(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);
    mLightDataBuffer = mRenderer->createBuffer(sizeof(LightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);
    mObjectDataBuffer = mRenderer->createBuffer(sizeof(ObjectData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    //  fill buffer data which is fixed from start and do not change
    {
        SceneData sceneData{.mViewMatrix = mCamera.getViewMatrix(),
            .mProjMatrix = mCamera.getProjMatrix(),
            .mViewProjMatrix = mCamera.getViewProjMatrix()};

        void* mappedSceneData = mSceneDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedSceneData, &sceneData, sizeof(SceneData));
    }
    {
        uint32_t lightCount = std::min(LightData::MAX_LIGHT_COUNT, mLights.size());
        LightData lightData{
            .mCameraPos = mCamera.getPosition(),
            .mLightCount = lightCount,
        };
        memcpy(lightData.mLights, mLights.data(), lightCount * sizeof(DirectionalLight));

        void* mappedSceneData = mLightDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedSceneData, &lightData, sizeof(LightData));
    }
}

MeshRenderComponent::MeshRenderComponent(const Mesh* mesh, const Node* owner) : mMesh(mesh), mOwner(owner)
{
}

void MeshRenderComponent::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix)
{
    glm::mat4 modelMat = parentTransformMatrix * mOwner->mTransform.mMatrix;
    glm::mat4 invTransMat = glm::inverse(glm::transpose(modelMat));

    //  bind buffers
    VkBuffer vertexBuffers[] = {mMesh->mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mMesh->mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    ObjectData pushConst{.model = modelMat, .invTransModel = invTransMat};

    //  draw!!
    for (const auto& surface : mMesh->mSurfaces)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, surface.mMaterial.mpBaseMaterial->mPipeline);

        //  bind scene data
        //  should optimize this later to not re-bind
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            surface.mMaterial.mpBaseMaterial->mPipelineLayout, 0, 1,
            &(mOwner->mScene->mSceneDescriptorSets[mOwner->mScene->mRenderer->getCurrentFrameId()]), 0, nullptr);

        vkCmdPushConstants(commandBuffer, surface.mMaterial.mpBaseMaterial->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(ObjectData), &pushConst);
        vkCmdDrawIndexed(commandBuffer, surface.mIndexCount, 1, surface.mStartIndex, 0, 0);
    }
}

void Node::render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix)
{
    //  render self if render component exists
    if (mRenderComponent != nullptr)
    {
        mRenderComponent->render(commandBuffer, parentTransformMatrix);
    }

    //  render children
    glm::mat4 transformMat = parentTransformMatrix * mTransform.mMatrix;
    for (Node* child : mChildren)
    {
        child->render(commandBuffer, transformMat);
    }
}

bool SceneInitializer::loadFromGltfFile(
    std::string_view filePath, BaseVulkanRenderer* renderer, Scene* scene, MeshAssetsBank* meshAssetsBank)
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
        const auto id = meshAssetsBank->mMeshes.size();
        meshAssetsBank->mMeshes[id] = std::make_unique<Mesh>();
        std::unique_ptr<Mesh>& newMesh = meshAssetsBank->mMeshes.at(id);
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
                newSurface.mIndexCount = indexAccessor.count;

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
            newSurface.mMaterial = renderer->getMaterial()->makeInstance();

            newMesh->mSurfaces.push_back(newSurface);
        }

        //  create mesh buffers
        renderer->createAndMapMeshBuffers(newMesh.get(), vertices, indices);
    }

    //  load nodes
    for (const fastgltf::Node& node : gltf.nodes)
    {
        const auto id = scene->mNodes.size();
        Node& newSceneNode = scene->mNodes[id];
        newSceneNode.mScene = scene;

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
                meshAssetsBank->mMeshes.at(node.meshIndex.value()).get(), &newSceneNode);
        }
    }

    //  load node scene structure
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node = gltf.nodes[i];
        Node& sceneNode = scene->mNodes.at(i);

        for (auto& c : node.children)
        {
            sceneNode.mChildren.push_back(&scene->mNodes[c]);
            scene->mNodes[c].mParent = &sceneNode;
        }
    }

    //  find root nodes, which are nodes that have no parent
    scene->findRootNodes();

    //  config camera
    //  will use default one here

    //  create lights
    scene->mLights.push_back(DirectionalLight{
        .mDirection = glm::normalize(glm::vec3{-1, -1, 1}
          ),
        .mColor = {1,  1,  1},
    });

    return true;
}

bool SceneInitializer::makeExampleScene(BaseVulkanRenderer* renderer, Scene* scene, MeshAssetsBank* meshAssetBank)
{
    //  vecters to hold indices and vertices for creating buffers
    // std::vector<uint32_t> indices;
    // std::vector<NormalVertex> vertices;
    // std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash> vertexToIndexMap;

    //  create meshes and save them in the mesh asset bank
    const Mesh* cubeMesh = createCubeMeshToBank(meshAssetBank, renderer);

    //  create scene structure
    //  create grid of cubes to fill the scene

    constexpr int resolution = 2;
    constexpr float gap = 2;
    // glm::vec3 pos{-gap * resolution / 2, -gap * resolution / 2, -gap * resolution / 2};
    // scene->mNodes.clear();
    // size_t idx = 0;
    // for (int z = 0; z < resolution; z++)
    // {
    //     for (int y = 0; y < resolution; y++)
    //     {
    //         for (int x = 0; x < resolution; x++)
    //         {
    //             Node& newNode = scene->mNodes[idx++];
    //             newNode.mScene = scene;
    //             newNode.mTransform = Base::Transform{
    //                 pos + glm::vec3{x * gap, y * gap, z * gap},
    //                   {0,       0,       0      },
    //                   {1,       1,       1      }
    //             };
    //             newNode.mRenderComponent = std::make_unique<MeshRenderComponent>(cubeMesh, &newNode);
    //         }
    //     }
    // }
    std::vector<glm::vec3> positions{
        {0, 0, 0},
        {2, 0, 0},
        {4, 0, 0},
        {0, 2, 0},
        {0, 4, 0},
        {0, 6, 0},
        {0, 0, 2},
        {0, 0, 4},
        {0, 0, 6},
        {0, 0, 8}
    };
    scene->mNodes.clear();
    for (int idx = 0; idx < positions.size(); idx++)
    {
        Node& newNode = scene->mNodes[idx];
        newNode.mScene = scene;
        newNode.mTransform = Base::Transform{
            positions[idx], {0, 0, 0},
             {1, 1, 1}
        };
        newNode.mRenderComponent = std::make_unique<MeshRenderComponent>(cubeMesh, &newNode);
    }

    scene->findRootNodes();

    //  config camera
    scene->mCamera = Camera(glm::vec3{5, 5, -5});

    //  create lights
    scene->mLights.push_back(DirectionalLight{
        .mDirection = glm::normalize(glm::vec3{-1, -1, 1}
          ),
        .mColor = {1,  1,  1},
    });

    return true;
}

void SceneInitializer::addVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec4& color,
    const glm::vec2& texCoord, std::vector<uint32_t>& indices, std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    NormalVertex newVertex{.mPosition = position, .mNormal = normal, .mColor = color, .mTexCoord = texCoord};

    const auto verIdxPair = vertexToIndexMap.find(newVertex);
    if (verIdxPair != vertexToIndexMap.end())
    {
        indices.push_back(verIdxPair->second);
    }
    else
    {
        uint32_t newIndex = vertices.size();
        vertices.emplace_back(newVertex);
        indices.emplace_back(newIndex);
        vertexToIndexMap[newVertex] = newIndex;
    }
}

void SceneInitializer::addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
    const glm::vec4& color, const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    //  p1 - - p3 - u
    //  |     /
    //  |   /
    //  | /
    //  p2
    //  |
    //  v

    glm::vec3 v12 = p2 - p1;
    glm::vec3 v13 = p3 - p1;
    glm::vec3 normal = glm::normalize(glm::cross(v12, v13));

    glm::vec2 tex1 = texCoordBase;
    glm::vec2 tex2 = texCoordBase + glm::vec2{0, scale};
    glm::vec2 tex3 = texCoordBase + glm::vec2{scale, 0};

    addVertex(p1, normal, color, tex1, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p3, normal, color, tex3, indices, vertices, vertexToIndexMap);
}

void SceneInitializer::addQuad(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4,
    const glm::vec4& color, const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    //  p1 - - p4 - u
    //  |     / |
    //  |   /   |
    //  | /     |
    //  p2 - - p3
    //  |
    //  v

    glm::vec3 v12 = p2 - p1;
    glm::vec3 v14 = p4 - p1;
    glm::vec3 normal = glm::normalize(glm::cross(v12, v14));

    glm::vec2 tex1 = texCoordBase;
    glm::vec2 tex2 = texCoordBase + glm::vec2{0, scale};
    glm::vec2 tex3 = texCoordBase + glm::vec2{scale, scale};
    glm::vec2 tex4 = texCoordBase + glm::vec2{scale, 0};

    addVertex(p1, normal, color, tex1, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p4, normal, color, tex4, indices, vertices, vertexToIndexMap);

    addVertex(p4, normal, color, tex4, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p3, normal, color, tex3, indices, vertices, vertexToIndexMap);
}

Mesh* SceneInitializer::createCubeMeshToBank(MeshAssetsBank* bank, BaseVulkanRenderer* renderer)
{
    const auto id = bank->mMeshes.size();
    bank->mMeshes[id] = std::make_unique<Mesh>();
    Mesh* newMesh = bank->mMeshes.at(id).get();
    newMesh->mName = "Cube";

    newMesh->mSurfaces.resize(1);
    Surface& cubeSurface = newMesh->mSurfaces[0];
    cubeSurface.mMaterial = renderer->getMaterial()->makeInstance();
    cubeSurface.mStartIndex = 0;

    std::vector<uint32_t> indices;
    std::vector<NormalVertex> vertices;
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash> vertexToIndexMap;

    constexpr glm::vec4 red{1.0f, 0.0, 0.0, 1.0};
    constexpr glm::vec4 green{0.0f, 1.0, 0.0, 1.0};
    constexpr glm::vec4 blue{0.0f, 0.0, 1.0, 1.0};
    constexpr glm::vec4 yellow{1.0f, 1.0, 0.0, 1.0};
    constexpr glm::vec4 fuchsia{1.0f, 0.0, 1.0, 1.0};
    constexpr glm::vec4 aqua{0.0f, 1.0, 1.0, 1.0};
    //  front   -z
    addQuad({-0.5, 0.5, -0.5}, {-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5}, aqua, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  right   +x
    addQuad({0.5, 0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, -0.5, 0.5}, {0.5, 0.5, 0.5}, red, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);
    //  up      +y
    addQuad({-0.5, 0.5, 0.5}, {-0.5, 0.5, -0.5}, {0.5, 0.5, -0.5}, {0.5, 0.5, 0.5}, green, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);
    //  left    -x
    addQuad({-0.5, 0.5, 0.5}, {-0.5, -0.5, 0.5}, {-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5}, yellow, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  bottom  -y
    addQuad({-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5}, {0.5, -0.5, -0.5}, fuchsia, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  back    +z
    addQuad({0.5, 0.5, 0.5}, {0.5, -0.5, 0.5}, {-0.5, -0.5, 0.5}, {-0.5, 0.5, 0.5}, blue, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);

    cubeSurface.mIndexCount = indices.size();
    renderer->createAndMapMeshBuffers(newMesh, vertices, indices);

    return newMesh;
}

} // namespace Bunny::Render
