#include "MaterialBank.h"

#include "Error.h"

namespace Bunny::Render
{
void MaterialBank::cleanup()
{
    mMaterialInstances.clear();

    for (auto& material : mMaterials)
    {
        material->cleanup();
    }

    mMaterials.clear();
}

void MaterialBank::addMaterial(std::unique_ptr<Material>&& material)
{
    const IdType matId = mMaterials.size();
    mMaterials.emplace_back(std::move(material));
    mMaterials[matId]->mId = matId;
}

void MaterialBank::addMaterialInstance(MaterialInstance materialInstance)
{
    const IdType instId = mMaterialInstances.size();
    mMaterialInstances.emplace_back(materialInstance);
    mMaterialInstances[instId].mId = instId;
}

const Material* MaterialBank::getMaterial(IdType materialId) const
{
    return mMaterials.at(materialId).get();
}

const MaterialInstance& MaterialBank::getMaterialInstance(IdType instanceId) const
{
    return mMaterialInstances.at(instanceId);
}

PbrMaterialBank::PbrMaterialBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

BunnyResult PbrMaterialBank::initialize()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayouts())

    return BUNNY_HAPPY;
}

void PbrMaterialBank::cleanup()
{
    mDeletionStack.Flush();
}

BunnyResult PbrMaterialBank::buildDescriptorSetLayouts()
{
    return BUNNY_HAPPY;
}

BunnyResult PbrMaterialBank::buildPipelineLayouts()
{
    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
