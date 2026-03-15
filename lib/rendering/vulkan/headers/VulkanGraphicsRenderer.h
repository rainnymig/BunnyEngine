#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"
#include "FunctionStack.h"
#include "SwapChainSupportDetails.h"
#include "BunnyGuard.h"

#include <volk.h>

#include <vector>
#include <array>
#include <span>

namespace Bunny::Render
{
class VulkanRenderResources;

class VulkanGraphicsRenderer
{
  public:
    class RenderHelper
    {
      public:
        RenderHelper(const VulkanGraphicsRenderer* renderer, Base::BunnyGuard<VulkanGraphicsRenderer> guard);
        ~RenderHelper();
        void beginRender();
        void finishRender();

        RenderHelper& setColorAttachments(const std::vector<VkImageView>& colorAttachments);
        RenderHelper& setUpdateDepth(bool updateDepth);
        RenderHelper& setClearColor(bool clearColor);
        RenderHelper& setClearColorValue(const VkClearValue& clearValue);
        RenderHelper& setMultiSample(bool multiSample, bool resolve);

      private:
        std::vector<VkImageView> mColorAttachmentViews;
        bool mUpdateDepth = true;
        bool mClearColor = true;
        VkClearValue mColorClearValue = {
            .color = {0.0f, 0.0f, 0.0f, 1.0f}
        };
        bool mMultiSample = false;
        bool mResolveMultiSample = false;

        bool mRenderBeginned = false;

        const VulkanGraphicsRenderer* mRenderer = nullptr;
    };
    friend class RenderHelper;

    VulkanGraphicsRenderer(VulkanRenderResources* renderResources);

    BunnyResult initialize();

    void beginRenderFrame();
    void finishRenderFrame();
    void beginRender(bool updateDepth) const;
    void beginRender(
        const std::vector<VkImageView>& colorAttachmentViews, bool updateDepth, bool clearColor = true) const;
    void finishRender() const;
    void beginImguiFrame();
    void finishImguiFrame();
    void finishImguiFrame(VkCommandBuffer commandBuffer, VkImageView targetImageView);

    void waitForRenderFinish();
    void cleanup();
    ~VulkanGraphicsRenderer();

    [[nodiscard]] RenderHelper getRenderHelper() const;

    VkCommandBuffer getCurrentCommandBuffer() const { return mFrameResources.at(mCurrentFrameId).mCommandBuffer; }
    uint32_t getCurrentFrameIdx() const { return mCurrentFrameId; }
    VkFormat getSwapChainImageFormat() const { return mSwapChainImageFormat; }
    VkFormat getDepthImageFormat() const { return mDepthImageFormat; }
    const AllocatedImage& getDepthImageResolved() const { return mFrameResources[mCurrentFrameId].mDepthImageResolved; }
    const AllocatedImage& getDepthImageResolved(uint32_t frameId) const
    {
        return mFrameResources[frameId].mDepthImageResolved;
    }
    const AllocatedImage& getMultiSampledColorImage() const
    {
        return mFrameResources[mCurrentFrameId].mMultiSampledColorImage;
    }
    const AllocatedImage& getColorImageResolved() const { return mFrameResources[mCurrentFrameId].mColorImageResolved; }
    VkExtent2D getSwapChainExtent() const { return mSwapChainExtent; }
    VkSampleCountFlagBits getRenderMultiSampleCount() const { return mRenderMultiSampleCount; }

  private:
    struct FrameRenderObject
    {
        VkCommandPool mCommandPool;
        VkCommandBuffer mCommandBuffer;
        VkSemaphore mSwapchainImageSemaphore;
        VkSemaphore mRenderFinishSemaphore;
        VkFence mFrameInflightFence;

        AllocatedImage mMultiSampledColorImage;
        AllocatedImage mColorImageResolved;
        AllocatedImage mDepthImage;
        AllocatedImage mDepthImageResolved;
    };

    BunnyResult initSwapChain();
    BunnyResult initFrameResources();
    BunnyResult initColorResources();
    BunnyResult initDepthResource();
    BunnyResult initImgui();

    BunnyResult createSwapChain();
    BunnyResult recreateSwapChain();
    void destroySwapChain();

    BunnyResult createColorResources();
    void destroyColorResources();

    BunnyResult createDepthResource();
    void destroyDepthResource();

    void queryMaxSupportedSampleCount();
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
    uint32_t mSwapchainImageIndex = 0;
    VkFormat mDepthImageFormat;

    VkSampleCountFlagBits mMaxSupportedSampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits mRenderMultiSampleCount = VK_SAMPLE_COUNT_4_BIT;

    std::array<FrameRenderObject, MAX_FRAMES_IN_FLIGHT> mFrameResources;
    uint32_t mCurrentFrameId = 0;

    bool mFrameBufferResized = false;

    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
