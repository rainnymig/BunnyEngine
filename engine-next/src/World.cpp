#include "World.h"

#include "VulkanRenderResources.h"
#include "Vertex.h"
#include "MaterialBank.h"
#include "Helper.h"
#include "Fundamentals.h"

#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <filesystem>
#include <algorithm>

namespace Bunny::Engine
{
WorldLoader::WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::MaterialBank* materialBank,
    Render::MeshBank<Render::NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mMaterialBank(materialBank),
      mMeshBank(meshBank)
{
}

void World::update(float deltaTime)
{
    const glm::mat4 rotMat = glm::eulerAngleY<float>(glm::pi<float>() * deltaTime / 4);

    auto transComps = mEntityRegistry.view<TransformComponent>();

    transComps.each([deltaTime, &rotMat](TransformComponent& transComp) {
        transComp.mTransform.mMatrix = rotMat * transComp.mTransform.mMatrix;
    });
}

BunnyResult WorldLoader::loadGltfToWorld(std::string_view filePath, World& outWorld)
{
    std::filesystem::path path(filePath);
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;
    auto loadedFile = fastgltf::GltfDataBuffer::FromPath(path);
    if (loadedFile.error() != fastgltf::Error::None)
    {
        return BUNNY_SAD;
    }
    fastgltf::Parser parser;
    auto parseResult = parser.loadGltf(loadedFile.get(), path.parent_path(), gltfOptions);
    if (parseResult.error() != fastgltf::Error::None)
    {
        return BUNNY_SAD;
    }
    fastgltf::Asset& gltf = parseResult.get();

    //  load materials

    //  load textures

    //  load meshes
    Render::loadMeshFromGltf(mMeshBank, mMaterialBank, gltf);

    //  save the mapping from fastgltf node idx to entity
    //  to convert the children
    std::vector<entt::entity> nodeIdxToEntity;
    nodeIdxToEntity.reserve(gltf.nodes.size());

    //  load world structure
    //  load game object local transforms
    for (const fastgltf::Node& node : gltf.nodes)
    {

        const auto nodeEntity = outWorld.mEntityRegistry.create();
        nodeIdxToEntity.push_back(nodeEntity);
        Base::Transform nodeTransform;

        //  load node (local) transform
        std::visit(fastgltf::visitor{[&nodeTransform](fastgltf::TRS trs) {
                                         glm::vec3 tl(trs.translation[0], trs.translation[1], trs.translation[2]);
                                         glm::quat rot(
                                             trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]);
                                         glm::vec3 sc(trs.scale[0], trs.scale[1], trs.scale[2]);

                                         glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                                         glm::mat4 rm = glm::toMat4(rot);
                                         glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                                         nodeTransform.mMatrix = tm * rm * sm;
                                     },
                       [&nodeTransform](fastgltf::math::fmat4x4 transformMat) {
                           memcpy(&nodeTransform.mMatrix, transformMat.data(), sizeof(transformMat));
                       }},
            node.transform);
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);

        //  load node mesh if present
        if (node.meshIndex.has_value())
        {
            std::string meshName{gltf.meshes.at(node.meshIndex.value()).name.c_str()};
            size_t meshId = mMeshBank->getMeshIdFromName(meshName);
            outWorld.mEntityRegistry.emplace<MeshComponent>(nodeEntity, meshId);
        }
    }

    //  load node scene structure
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
        fastgltf::Node& node = gltf.nodes[i];
        const auto nodeEntity = nodeIdxToEntity.at(i);

        for (auto c : node.children)
        {
            HierarchyComponent& childHierarchyComp =
                outWorld.mEntityRegistry.emplace_or_replace<HierarchyComponent>(nodeIdxToEntity.at(c));
            childHierarchyComp.mParent = nodeEntity;
        }
    }

    return BUNNY_HAPPY;
}

BunnyResult WorldLoader::loadTestWorld(World& outWorld)
{
    //  create meshes and save them in the mesh asset bank
    const Render::IdType meshId = Render::createCubeMeshToBank(mMeshBank, mMaterialBank->getDefaultMaterialId());
    mMeshBank->buildMeshBuffers();

    //  create scene structure
    //  create grid of cubes to fill the scene
    constexpr int resolution = 4;
    constexpr float gap = 2;
    std::vector<glm::vec3> positions;
    glm::vec3 startingPos{-gap * resolution / 2, -gap * resolution / 2, -gap * resolution / 2};
    size_t idx = 0;
    for (int z = 0; z < resolution; z++)
    {
        for (int y = 0; y < resolution; y++)
        {
            for (int x = 0; x < resolution; x++)
            {
                positions.emplace_back(
                    glm::vec3{startingPos.x + x * gap, startingPos.y + y * gap, startingPos.z + z * gap});
            }
        }
    }
    // std::vector<glm::vec3> positions{
    //     {0, 0, 0},
    //     {2, 0, 0},
    //     {4, 0, 0},
    //     {0, 2, 0},
    //     {0, 4, 0},
    //     {0, 6, 0},
    //     {0, 0, 2},
    //     {0, 0, 4},
    //     {0, 0, 6},
    //     {0, 0, 8}
    // };

    //  nodes
    for (int idx = 0; idx < positions.size(); idx++)
    {
        Base::Transform nodeTransform(positions[idx], {0, 0, 0}, {1, 1, 1});
        const auto nodeEntity = outWorld.mEntityRegistry.create();
        outWorld.mEntityRegistry.emplace<TransformComponent>(nodeEntity, nodeTransform);
        outWorld.mEntityRegistry.emplace<MeshComponent>(nodeEntity, meshId);
    }

    //  camera
    {
        const auto cameraEntity = outWorld.mEntityRegistry.create();
        Render::Camera camera(glm::vec3{0, 10, -15});
        outWorld.mEntityRegistry.emplace<CameraComponent>(cameraEntity, camera);
    }

    //  light
    {
        const auto lightEntity = outWorld.mEntityRegistry.create();
        Render::DirectionalLight dirLight{
            .mDirection = glm::normalize(glm::vec3{0, -1, -2}
              ),
            .mColor = {1, 1,  1 },
        };
        outWorld.mEntityRegistry.emplace<DirectionLightComponent>(lightEntity, dirLight);
    }

    return BUNNY_HAPPY;
}

WorldRenderDataTranslator::WorldRenderDataTranslator(const Render::VulkanRenderResources* vulkanResources,
    const Render::MeshBank<Render::NormalVertex>* meshBank, const Render::MaterialBank* materialBank)
    : mVulkanResources(vulkanResources),
      mMeshBank(meshBank),
      mMaterialBank(materialBank)
{
}

BunnyResult WorldRenderDataTranslator::initialize()
{
    constexpr VkDeviceSize MAX_OBJECT_BUFFER_SIZE = sizeof(Render::ObjectData) * World::MAX_OBJECT_COUNT;

    mObjectData.resize(World::MAX_OBJECT_COUNT);

    mObjectDataBuffer = mVulkanResources->createBuffer(MAX_OBJECT_BUFFER_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, //  storage buffer?
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mSceneDataBuffer = mVulkanResources->createBuffer(sizeof(Render::SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mLightDataBuffer = mVulkanResources->createBuffer(sizeof(Render::LightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    return BUNNY_HAPPY;
}

BunnyResult WorldRenderDataTranslator::translateSceneData(const World* world)
{
    const auto camComps = world->mEntityRegistry.view<CameraComponent>();

    if (camComps.empty())
    {
        return BUNNY_SAD;
    }

    for (auto [entity, cam] : camComps.each())
    {
        mSceneData.mProjMatrix = cam.mCamera.getProjMatrix();
        mSceneData.mViewMatrix = cam.mCamera.getViewMatrix();
        mSceneData.mViewProjMatrix = cam.mCamera.getViewProjMatrix();
        mLightData.mCameraPos = cam.mCamera.getPosition();
        break;
    }

    {
        void* mappedSceneData = mSceneDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedSceneData, &mSceneData, sizeof(Render::SceneData));
    }

    const auto lightComps = world->mEntityRegistry.view<DirectionLightComponent>();
    if (lightComps.empty())
    {
        return BUNNY_SAD;
    }

    size_t idx = 0;
    for (auto [entity, light] : lightComps.each())
    {
        mLightData.mLights[idx].mColor = light.mLight.mColor;
        mLightData.mLights[idx].mDirection = light.mLight.mDirection;
        idx++;
    }
    mLightData.mLightCount = idx;

    {
        void* mappedLightData = mLightDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedLightData, &mLightData, sizeof(Render::LightData));
    }

    return BUNNY_HAPPY;
}

BunnyResult WorldRenderDataTranslator::translateObjectData(const World* world)
{
    mRenderBatches.clear();

    auto meshTransComp = world->mEntityRegistry.view<MeshComponent, TransformComponent>();

    size_t idx = 0;
    size_t countInBatch = 0;
    Render::RenderBatch currentBatch{.mMaterialInstanceId = 0,
        .mMeshId = Render::BUNNY_INVALID_ID,
        .mObjectBuffer = &mObjectDataBuffer,
        .mInstanceCount = 0};
    for (auto [entity, mesh, transform] : meshTransComp.each())
    {
        if (mesh.mMeshId != currentBatch.mMeshId)
        {
            if (countInBatch > 0)
            {
                currentBatch.mInstanceCount = countInBatch;
                mRenderBatches.push_back(currentBatch);

                countInBatch = 0;
            }
            currentBatch.mMeshId = mesh.mMeshId;
            currentBatch.mMaterialInstanceId = mMaterialBank->getDefaultMaterialId();
        }

        Render::ObjectData& obj = mObjectData[idx];
        obj.model = getEntityGlobalTransform(world->mEntityRegistry, entity, transform.mTransform.mMatrix);
        obj.invTransModel = glm::transpose(glm::inverse(obj.model));

        countInBatch++;
        idx++;
    }
    if (countInBatch > 0)
    {
        currentBatch.mInstanceCount = countInBatch;
        mRenderBatches.push_back(currentBatch);
    }

    {
        void* mappedObjectData = mObjectDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedObjectData, mObjectData.data(), idx * sizeof(Render::ObjectData));
    }

    return BUNNY_HAPPY;
}

void WorldRenderDataTranslator::cleanup()
{
    mVulkanResources->destroyBuffer(mObjectDataBuffer);
    mVulkanResources->destroyBuffer(mSceneDataBuffer);
    mVulkanResources->destroyBuffer(mLightDataBuffer);
}

glm::mat4x4 WorldRenderDataTranslator::getEntityGlobalTransform(
    const entt::registry& registry, entt::entity entity, const glm::mat4x4& transformMat)
{
    if (const auto* hierComp = registry.try_get<HierarchyComponent>(entity);
        hierComp && hierComp->mParent != entt::null)
    {
        const auto parentNode = hierComp->mParent;
        const TransformComponent& transComp = registry.get<TransformComponent>(parentNode);
        glm::mat4x4 combinedTrans = transComp.mTransform.mMatrix * transformMat;
        return getEntityGlobalTransform(registry, parentNode, combinedTrans);
    }
    else
    {
        return transformMat;
    }
}

void BaseWorld::update(float deltaTime)
{
}

} // namespace Bunny::Engine
