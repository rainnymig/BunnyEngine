#include "MaterialBank.h"

#include "Material.h"

namespace Bunny::Render
{
    const MaterialInstance& MaterialBank::getMaterialInstance(IdType instanceId) const
    {
        return mMaterialInstances.at(instanceId);
    }
} // namespace Bunny::Render
