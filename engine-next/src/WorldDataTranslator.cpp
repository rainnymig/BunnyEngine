#include "WorldDataTranslator.h"

#include "WorldComponents.h"

namespace Bunny::Engine
{

WorldRenderDataTranslator::WorldRenderDataTranslator(
    const Render::VulkanRenderResources* vulkanResources, const Render::MeshBank<Render::NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mMeshBank(meshBank)
{
}

BunnyResult WorldRenderDataTranslator::initialize()
{
    mSceneDataBuffer = mVulkanResources->createBuffer(sizeof(Render::SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mLightDataBuffer = mVulkanResources->createBuffer(sizeof(Render::LightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    //  PBR
    mPbrCameraBuffer = mVulkanResources->createBuffer(sizeof(Render::PbrCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mPbrLightBuffer = mVulkanResources->createBuffer(sizeof(Render::PbrLightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    return BUNNY_HAPPY;
}

BunnyResult WorldRenderDataTranslator::updateSceneData(const World* world)
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

BunnyResult WorldRenderDataTranslator::updatePbrWorldData(const World* world)
{
    const auto camComps = world->mEntityRegistry.view<PbrCameraComponent>();

    if (camComps.empty())
    {
        return BUNNY_SAD;
    }

    for (auto [entity, cam] : camComps.each())
    {
        mPbrCameraData.mPosition = cam.mCamera.getPosition();
        mPbrCameraData.mViewProjMat = cam.mCamera.getViewProjMatrix();
        mPbrCameraData.mExposure = cam.mCamera.getExposure();
        break;
    }

    {
        void* mappedCameraData = mPbrCameraBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedCameraData, &mPbrCameraData, sizeof(Render::PbrCameraData));
    }

    const auto lightComps = world->mEntityRegistry.view<PbrLightComponent>();
    if (lightComps.empty())
    {
        return BUNNY_SAD;
    }

    size_t idx = 0;
    for (auto [entity, light] : lightComps.each())
    {
        mPbrLightData.mLights[idx] = light.mLight;
        idx++;
    }
    mPbrLightData.mLightCount = idx;

    {
        void* mappedLightData = mPbrLightBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedLightData, &mPbrLightData, sizeof(Render::LightData));
    }

    return BUNNY_HAPPY;
}

BunnyResult WorldRenderDataTranslator::updateObjectData(const World* world)
{
    auto meshTransComp = world->mEntityRegistry.view<MeshComponent, TransformComponent>();
    size_t idx = 0;
    for (auto [entity, mesh, transform] : meshTransComp.each())
    {
        glm::mat4 modelMat;
        glm::vec3 scale;
        getEntityGlobalTransform(
            world->mEntityRegistry, entity, transform.mTransform.mMatrix, transform.mTransform.mScale, modelMat, scale);
        glm::mat4 invTransModel = glm::transpose(glm::inverse(modelMat));
        Render::ObjectData& obj = mObjectData[idx];
        obj.model = modelMat;
        obj.invTransModel = invTransModel;
        obj.scale = scale;
        obj.meshId = mesh.mMeshId;
        obj.materialId = mesh.mMaterialId;
        idx++;
    }

    {
        void* mappedObjectData = mObjectDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedObjectData, mObjectData.data(), mObjectData.size() * sizeof(Render::ObjectData));
    }

    return BUNNY_HAPPY;
}

BunnyResult WorldRenderDataTranslator::initObjectDataBuffer(const World* world)
{
    mObjectData.clear();
    mMeshInstanceCounts.clear();

    auto meshTransComp = world->mEntityRegistry.view<MeshComponent, TransformComponent>();
    for (auto [entity, mesh, transform] : meshTransComp.each())
    {
        glm::mat4 modelMat;
        glm::vec3 scale;
        getEntityGlobalTransform(
            world->mEntityRegistry, entity, transform.mTransform.mMatrix, transform.mTransform.mScale, modelMat, scale);
        glm::mat4 invTransModel = glm::transpose(glm::inverse(modelMat));
        mObjectData.emplace_back(modelMat, invTransModel, scale, mesh.mMeshId, mesh.mMaterialId);

        mMeshInstanceCounts[mesh.mMeshId]++;
    }

    //  rebuild object data buffer
    if (mObjectDataBuffer.mBuffer != nullptr)
    {
        mVulkanResources->destroyBuffer(mObjectDataBuffer);
    }
    mObjectDataBuffer = mVulkanResources->createBuffer(mObjectData.size() * sizeof(Render::ObjectData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        VMA_MEMORY_USAGE_AUTO);

    {
        void* mappedObjectData = mObjectDataBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedObjectData, mObjectData.data(), mObjectData.size() * sizeof(Render::ObjectData));
    }

    return BUNNY_HAPPY;
}

void WorldRenderDataTranslator::cleanup()
{
    mVulkanResources->destroyBuffer(mObjectDataBuffer);
    mVulkanResources->destroyBuffer(mSceneDataBuffer);
    mVulkanResources->destroyBuffer(mLightDataBuffer);
    mVulkanResources->destroyBuffer(mPbrCameraBuffer);
    mVulkanResources->destroyBuffer(mPbrLightBuffer);
}

void WorldRenderDataTranslator::getEntityGlobalTransform(const entt::registry& registry, entt::entity entity,
    const glm::mat4x4& transform, const glm::vec3& scale, glm::mat4x4& outTransform, glm::vec3& outScale)
{
    if (const auto* hierComp = registry.try_get<HierarchyComponent>(entity);
        hierComp && hierComp->mParent != entt::null)
    {
        const auto parentNode = hierComp->mParent;
        const TransformComponent& transComp = registry.get<TransformComponent>(parentNode);
        glm::mat4x4 combinedTrans = transComp.mTransform.mMatrix * transform;
        glm::vec3 combinedScale = transComp.mTransform.mScale * scale;
        getEntityGlobalTransform(registry, parentNode, combinedTrans, combinedScale, outTransform, outScale);
    }
    else
    {
        outTransform = transform;
        outScale = scale;
    }
}

} // namespace Bunny::Engine
