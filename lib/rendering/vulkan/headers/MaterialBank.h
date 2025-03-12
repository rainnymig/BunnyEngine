#pragma once

#include "Fundamentals.h"
#include "Material.h"

#include <memory>
#include <unordered_map>

namespace Bunny::Render
{

class MaterialBank
{
public:

    void cleanup();

    void addMaterial(std::unique_ptr<Material>&& material);
    void addMaterialInstance(MaterialInstance materialInstance);

    const MaterialInstance& getMaterialInstance(IdType instanceId) const;
    
    //  helper function to return a default material
    const IdType getDefaultMaterialId() const { return mDefaultMaterialInstanceId; }

private:
    std::unordered_map<IdType, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<IdType, MaterialInstance> mMaterialInstances;
    IdType mDefaultMaterialInstanceId;
};
} // namespace Bunny::Render
