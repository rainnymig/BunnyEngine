#include "Helper.h"

#include "ErrorCheck.h"

namespace Bunny::Render
{
VkCommandPoolCreateInfo makeCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo makeCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t bufferCount)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = commandPool;
    info.commandBufferCount = bufferCount;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferSubmitInfo makeCommandBufferSubmitInfo(VkCommandBuffer commandBuffer)
{
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = commandBuffer;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 makeSubmitInfo2(VkCommandBufferSubmitInfo* cmdBufferSubmit, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmdBufferSubmit;

    return info;
}

VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entryPoint)
{
    VkPipelineShaderStageCreateInfo info{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = stage,
        .module = shaderModule,
        .pName = entryPoint};
    return info;
}

VkRayTracingShaderGroupCreateInfoKHR makeRayTracingShaderGroupCreateInfoKHR()
{
    VkRayTracingShaderGroupCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    createInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    createInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    createInfo.generalShader = VK_SHADER_UNUSED_KHR;
    createInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    return createInfo;
}

VkRenderingAttachmentInfo makeAttachmentInfo(
    VkImageView view, VkImageLayout layout, VkClearValue* clearValue, VkImageView resolveView)
{
    bool shouldClear = clearValue != VK_NULL_HANDLE;
    bool shouldResolve = resolveView != VK_NULL_HANDLE;

    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.pNext = nullptr;

    attachmentInfo.imageView = view;
    attachmentInfo.imageLayout = layout;

    if (shouldResolve)
    {
        attachmentInfo.resolveImageView = resolveView;
        //  for now use the same layout as the main image
        //  maybe they can be different later
        attachmentInfo.resolveImageLayout = layout;
        attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    if (shouldClear)
    {
        attachmentInfo.clearValue = *clearValue;
    }

    attachmentInfo.loadOp = shouldClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentInfo.storeOp = shouldResolve ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;

    return attachmentInfo;
}

VkRenderingInfo makeRenderingInfo(VkExtent2D renderExtent, uint32_t colorAttachmentCount,
    VkRenderingAttachmentInfo* colorAttachments, VkRenderingAttachmentInfo* depthAttachment)
{
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;

    renderInfo.renderArea = VkRect2D{
        VkOffset2D{0, 0},
        renderExtent
    };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = colorAttachmentCount;
    renderInfo.pColorAttachments = colorAttachments;
    renderInfo.pDepthAttachment = depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    return renderInfo;
}

VkBufferMemoryBarrier makeBufferMemoryBarrier(VkBuffer buffer, uint32_t queueIndex)
{
    VkBufferMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcQueueFamilyIndex = queueIndex;
    barrier.dstQueueFamilyIndex = queueIndex;
    barrier.pNext = nullptr;

    return barrier;
}

VkImageMemoryBarrier makeImageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier result = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};

    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.oldLayout = oldLayout;
    result.newLayout = newLayout;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange.aspectMask = aspectMask;
    result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return result;
}

VkPipelineColorBlendAttachmentState makePipelineColorBlendAttachmentState(
    VkBlendFactor sourceBlendFactor, VkBlendFactor destBlendFactor, VkBlendOp blendOp)
{
    //  for now we can use the same blend factors for both color and alpha
    return VkPipelineColorBlendAttachmentState{
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = sourceBlendFactor,
        .dstColorBlendFactor = destBlendFactor,
        .colorBlendOp = blendOp,
        .srcAlphaBlendFactor = sourceBlendFactor,
        .dstAlphaBlendFactor = destBlendFactor,
        .alphaBlendOp = blendOp,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
}

VkPipelineColorBlendAttachmentState makeNoBlendAttachmentState()
{
    return VkPipelineColorBlendAttachmentState{
        .blendEnable = VK_FALSE,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

uint32_t findPreviousPow2(uint32_t val)
{
    uint32_t result = 1;

    while (result * 2 < val)
    {
        result *= 2;
    }

    return result;
}

bool hasFlag(VkFlags testFlag, VkFlags targetFlag)
{
    return (testFlag & targetFlag) == targetFlag;
}

VkTransformMatrixKHR convertToVkTransformMatrix(const glm::mat4& glmMat)
{
    //  glm::mat4 -> column major
    //  VkTransformMatrixKHR -> row major

    glm::mat4 transposedMat = glm::transpose(glmMat);
    VkTransformMatrixKHR vkMat;
    memcpy(&vkMat, &transposedMat, sizeof(VkTransformMatrixKHR));
    return vkMat;
}

} // namespace Bunny::Render