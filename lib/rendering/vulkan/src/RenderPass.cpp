#include "RenderPass.h"

#include "Material.h"
#include "MaterialBank.h"
#include "MeshBank.h"
#include "VulkanGraphicsRenderer.h"
#include "VulkanRenderResources.h"

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
void ForwardPass::initializePass()
{
}

void ForwardPass::renderBatch(const RenderBatch& batch)
{
    const MaterialInstance& matInstance = mMaterialBank->getMaterialInstance(batch.mMaterialInstanceId);

    vkCmdBindPipeline(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, matInstance.mpBaseMaterial->mPipeline);


    if (matInstance.mDescriptorSet != nullptr)
    {
        //  set 2 is material data, so start set is 2
        vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, 
        matInstance.mpBaseMaterial->mPipelineLayout, 1, 1, &matInstance.mDescriptorSet, 0, nullptr);
    }

    //  bind object data
    //  set 1 is object data

    //  draw indexed indirect
    //  temp
    //  should draw all meshes (surfaces) that has the same pipeline (material) and desc set (material instance)
    vkCmdDrawIndexedIndirect(mRenderer->getCurrentCommandBuffer(), nullptr, 0, 0, 1);
}
} // namespace Bunny::Render
