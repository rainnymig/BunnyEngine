#include "MaterialBank.h"


namespace Bunny::Render
{
void MaterialBank::cleanup()
{
    mMaterialInstances.clear();

    for (auto& [id, material] : mMaterials)
    {
        material->cleanup();
    }

    mMaterials.clear();
}

void MaterialBank::addMaterial(std::unique_ptr<Material>&& material)
{
    mMaterials[material->getId()] = std::move(material);
}

void MaterialBank::addMaterialInstance(MaterialInstance materialInstance)
{
    //  helper
    //  take the first material instance as default material instance
    if (mMaterialInstances.empty())
    {
        mDefaultMaterialInstanceId = materialInstance.mId;
    }

    mMaterialInstances[materialInstance.mId] = materialInstance;
}

const MaterialInstance& MaterialBank::getMaterialInstance(IdType instanceId) const
{
    return mMaterialInstances.at(instanceId);
}
} // namespace Bunny::Render
