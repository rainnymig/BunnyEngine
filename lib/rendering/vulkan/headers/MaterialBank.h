#pragma once

#include "Fundamentals.h"

#include "unordered_map"

namespace Bunny::Render
{

class MaterialInstance;
class Material;

class MaterialBank
{
public:
    const MaterialInstance& getMaterialInstance(IdType instanceId) const;

private:
    std::unordered_map<IdType, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<IdType, MaterialInstance> mMaterialInstances;
};
} // namespace Bunny::Render
