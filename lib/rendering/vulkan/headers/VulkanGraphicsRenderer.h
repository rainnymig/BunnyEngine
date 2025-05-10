#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"
#include "FunctionStack.h"
#include "SwapChainSupportDetails.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <span>

namespace Bunny::Render
{
class VulkanRenderResources;

class VulkanGraphicsRenderer
{
  public:
    VulkanGraphicsRenderer(VulkanRenderResources* renderResources);

    BunnyResult initialize();

    void beginRenderFrame();
    void finishRenderFrame();
    void beginRender(bool updateDepth) const;
    void beginRender(const std::vector<VkImageView>& colorAttachmentViews, bool updateDepth) const;
    void finishRender() const;
    void beginImguiFrame();
    void finishImguiFrame();
    void finishImguiFrame(VkCommandBuffer commandBuffer, VkImageView targetImageView);

    void waitForRenderFinish();
    void cleanup();
    ~VulkanGraphicsRenderer();

    VkCommandBuffer getCurrentCommandBuffer() const { return mFrameResources.at(mCurrentFrameId).mCommandBuffer; }
    uint32_t getCurrentFrameIdx() const { return mCurrentFrameId; }
    VkFormat getSwapChainImageFormat() const { return mSwapChainImageFormat; }
    VkFormat getDepthImageFormat() const { return mDepthImage.mFormat; }
    const AllocatedImage& getDepthImage() const { return mDepthImage; }
    VkExtent2D getSwapChainExtent() const { return mSwapChainExtent; }

  private:
    struct FrameRenderObject
    {
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;
        VkSemaphore mSwapchainImageSemaphore;
        VkSemaphore mRenderFinishSemaphore;
        VkFence mFrameInflightFence;
    };

    BunnyResult initSwapChain();
    BunnyResult initFrameResources();
    BunnyResult initDepthResource();
    BunnyResult initImgui();

    BunnyResult createSwapChain();
    BunnyResult recreateSwapChain();
    void destroySwapChain();

    BunnyResult createDepthResource();
    void destroyDepthResource();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    VkFormat findDepthFormat() const;

    VulkanRenderResources* mRenderResources = nullptr;

    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    AllocatedImage mDepthImage;
    uint32_t mSwapchainImageIndex = 0;

    std::array<FrameRenderObject, MAX_FRAMES_IN_FLIGHT> mFrameResources;
    uint32_t mCurrentFrameId = 0;

    bool mFrameBufferResized = false;

    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
