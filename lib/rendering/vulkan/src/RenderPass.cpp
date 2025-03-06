#include "RenderPass.h"

#include "MeshBank.h"

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
void ForwardPass::initializePass()
{
}

void ForwardPass::renderBatch(const std::vector<RenderBatch>& batches)
{
    //  batches should be already sorted by material and then material instance
    //  otherwise should sort first

    IdType lastMaterialId;
    IdType lastMaterialInstanceId;
    for (const RenderBatch& batch : batches)
    {
        //  bind material
    }
}
} // namespace Bunny::Render
