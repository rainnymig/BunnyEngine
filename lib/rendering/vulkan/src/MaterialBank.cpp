#include "MaterialBank.h"

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
} // namespace Bunny::Render
