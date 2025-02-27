#include "VulkanGraphicsRenderer.h"

#include "VulkanRenderResources.h"
#include "Window.h"
#include "Helper.h"
#include "Error.h"
#include "ErrorCheck.h"

#include <algorithm>
#include <cassert>

namespace Bunny::Render
{
VulkanGraphicsRenderer::VulkanGraphicsRenderer(VulkanRenderResources* renderResources)
    : mRenderResources(renderResources)
{
    assert(mRenderResources != nullptr);
}

BunnyResult VulkanGraphicsRenderer::initialize()
{

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initSwapChain());
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initFrameResources());
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDepthResource());

    return BUNNY_HAPPY;
}

void VulkanGraphicsRenderer::render(float deltaTime)
{
    VkDevice device = mRenderResources->getDevice();
    FrameRenderObject& currentFrame = mFrameResources[mCurrentFrameId];

    //  wait for previous frame to finish rendering
    vkWaitForFences(device, 1, &currentFrame.mFrameInflightFence, VK_TRUE, UINT64_MAX);

    //  wait for image in the swap chain to be available before acquiring it
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device, mSwapChain, UINT64_MAX, currentFrame.mSwapchainImageSemaphore, VK_NULL_HANDLE, &imageIndex);

    //  recreate swap chain when necessary
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        PRINT_AND_RETURN("failed to acquire swap chain image!")
    }

    //  reset fence only after image is successfully acquired
    vkResetFences(device, 1, &currentFrame.mFrameInflightFence);

    VkCommandBuffer cmdBuf = currentFrame.mCommandBuffer;
    //  reset command buffer
    vkResetCommandBuffer(cmdBuf, 0);

    //  begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VK_HARD_CHECK(vkBeginCommandBuffer(cmdBuf, &beginInfo))

    mRenderResources->transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkClearValue colorClearValue = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f}
    };
    VkRenderingAttachmentInfo colorAttachment =
        makeColorAttachmentInfo(mSwapChainImageViews[imageIndex], &colorClearValue);
    VkRenderingAttachmentInfo depthAttachment = makeDepthAttachmentInfo(mDepthImage.mImageView);

    VkRenderingInfo renderInfo = makeRenderingInfo(mSwapChainExtent, &colorAttachment, &depthAttachment);

    //  update dynamic states (viewport, scissors)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(mSwapChainExtent.width);
    viewport.height = static_cast<float>(mSwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = mSwapChainExtent;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    //  begin render
    vkCmdBeginRendering(cmdBuf, &renderInfo);


    //  end render
    vkCmdEndRendering(cmdBuf);

    //  run render ui

    mRenderResources->transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //  end command buffer
    VK_HARD_CHECK(vkEndCommandBuffer(cmdBuf))

    //  submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {currentFrame.mSwapchainImageSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    VkSemaphore signalSemaphores[] = {currentFrame.mRenderFinishSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_HARD_CHECK(vkQueueSubmit(mRenderResources->getGraphicQueue().mQueue, 1, &submitInfo, currentFrame.mFrameInflightFence))

    //  present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {mSwapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    result = vkQueuePresentKHR(mRenderResources->getPresentQueue().mQueue, &presentInfo);

    //  recreate swap chain when necessary
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFrameBufferResized)
    {
        mFrameBufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        PRINT_AND_RETURN("failed to present swap chain image!")
    }

    mCurrentFrameId = (mCurrentFrameId + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanGraphicsRenderer::cleanup()
{
    //  wait for rendering operations to finish before cleaning up
    if (mRenderResources->getDevice() != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(mRenderResources->getDevice());
    }

    mDeletionStack.Flush();
}

VulkanGraphicsRenderer::~VulkanGraphicsRenderer()
{
    cleanup();
}

BunnyResult VulkanGraphicsRenderer::initSwapChain()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createSwapChain());
    mDeletionStack.AddFunction([this]() { destroySwapChain(); });
    return BUNNY_HAPPY;
}

BunnyResult VulkanGraphicsRenderer::initFrameResources()
{
    assert(mRenderResources->getGraphicQueue().mQueueFamilyIndex.has_value());

    //  semaphore and fence create infos, they can be reused in every frame
    //  probably the simplest create infos!!
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //  initialize as signaled to not block the rendering of first frame

    //  create 1 command pool and 1 command buffer for each frame in flight
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        //  create command pool
        VkCommandPoolCreateInfo poolInfo =
            makeCommandPoolCreateInfo(mRenderResources->getGraphicQueue().mQueueFamilyIndex.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateCommandPool(mRenderResources->getDevice(), &poolInfo, nullptr, &mFrameResources[i].mCommandPool))

        //  create command buffer
        VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(mFrameResources[i].mCommandPool, 1);
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkAllocateCommandBuffers(mRenderResources->getDevice(), &allocInfo, &mFrameResources[i].mCommandBuffer))

        //  create sync objects
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateSemaphore(mRenderResources->getDevice(), &semaphoreInfo, nullptr, &mFrameResources[i].mSwapchainImageSemaphore))
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateSemaphore(mRenderResources->getDevice(), &semaphoreInfo, nullptr, &mFrameResources[i].mRenderFinishSemaphore))
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateFence(mRenderResources->getDevice(), &fenceInfo, nullptr, &mFrameResources[i].mFrameInflightFence))


        mDeletionStack.AddFunction([this, i](){
            vkDestroyFence(mRenderResources->getDevice(), mFrameResources[i].mFrameInflightFence, nullptr);
            vkDestroySemaphore(mRenderResources->getDevice(), mFrameResources[i].mSwapchainImageSemaphore, nullptr);
            vkDestroySemaphore(mRenderResources->getDevice(), mFrameResources[i].mRenderFinishSemaphore, nullptr);
            vkDestroyCommandPool(mRenderResources->getDevice(), mFrameResources[i].mCommandPool, nullptr);
        });
    }

    return BUNNY_HAPPY;
}

BunnyResult VulkanGraphicsRenderer::initDepthResource()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createDepthResource())
    mDeletionStack.AddFunction([this](){ destroyDepthResource(); });
    return BUNNY_HAPPY;
}

BunnyResult VulkanGraphicsRenderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mRenderResources->getPhysicalDevice(), mRenderResources->getSurface());
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    vkb::SwapchainBuilder swapchainBuilder{mRenderResources->getPhysicalDevice(), mRenderResources->getDevice(), mRenderResources->getSurface()};

    vkb::Swapchain vkbSwapchain =
        swapchainBuilder.set_desired_format(surfaceFormat)
            .set_desired_present_mode(presentMode)
            .set_desired_extent(extent.width, extent.height)
            //   .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT) //  transfer destination
            .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) //  direct render into swapchain image
            .build()
            .value();

    mSwapChainExtent = vkbSwapchain.extent;
    mSwapChain = vkbSwapchain.swapchain;
    mSwapChainImages = vkbSwapchain.get_images().value();
    mSwapChainImageViews = vkbSwapchain.get_image_views().value();
    mSwapChainImageFormat = surfaceFormat.format;

    return BUNNY_HAPPY;
}

BunnyResult VulkanGraphicsRenderer::recreateSwapChain()
{
    int width = 0, height = 0;
    mRenderResources->getWindow()->getFrameBufferSizeNotMinimized(width, height);

    vkDeviceWaitIdle(mRenderResources->getDevice());

    destroyDepthResource();
    destroySwapChain();

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createSwapChain())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createDepthResource())

    return BUNNY_HAPPY;
}

void VulkanGraphicsRenderer::destroySwapChain()
{
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mRenderResources->getDevice(), imageView, nullptr);
    }
    vkDestroySwapchainKHR(mRenderResources->getDevice(), mSwapChain, nullptr);
}

BunnyResult VulkanGraphicsRenderer::createDepthResource()
{
    VkFormat depthFormat = findDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        PRINT_AND_RETURN_VALUE("Can not find suitable depth image format", BUNNY_SAD)
    }

    mDepthImage = mRenderResources->createImage({mSwapChainExtent.width, mSwapChainExtent.height, 1}, depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

    //  transition the depth image layout
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mRenderResources->immediateTransitionImageLayout(mDepthImage.mImage, mDepthImage.mFormat, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))

    return BUNNY_HAPPY;
}

void VulkanGraphicsRenderer::destroyDepthResource()
{
    mRenderResources->destroyImage(mDepthImage);
}

SwapChainSupportDetails VulkanGraphicsRenderer::querySwapChainSupport(
    VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    SwapChainSupportDetails details;

    //  swap chain capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    //  swap chain supported formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    //  swap chain present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VulkanGraphicsRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    if (availableFormats.empty())
    {
        PRINT_AND_ABORT("The swap chain does not support any format");
    }

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanGraphicsRenderer::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) const
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanGraphicsRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width;
        int height;
        mRenderResources->getWindow()->getFrameBufferSize(width, height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkFormat Bunny::Render::VulkanGraphicsRenderer::findDepthFormat() const
{
    std::array<VkFormat, 3> candidates{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return mRenderResources->findSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace Bunny::Render
