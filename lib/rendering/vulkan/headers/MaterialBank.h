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

    const Material* getMaterial(IdType materialId) const;
    const MaterialInstance& getMaterialInstance(IdType instanceId) const;

    //  helper function to return a default material
    constexpr IdType getDefaultMaterialId() const { return 0; }
    constexpr IdType getDefaultMaterialInstanceId() const { return 0; }

  private:
    std::vector<std::unique_ptr<Material>> mMaterials;
    std::vector<MaterialInstance> mMaterialInstances;
};
} // namespace Bunny::Render
