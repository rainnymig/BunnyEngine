#pragma once

#include <Renderer.h>

#include "Descriptor.h"
#include "Fundamentals.h"
#include "SwapChainSupportDetails.h"
#include "Utils.h"
#include "Vertex.h"
#include "BaseVulkanRenderer.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <array>
#include <functional>
#include <span>
#include <optional>

namespace Bunny::Render
{
class VulkanRendererNext : public BaseVulkanRenderer
{
  public:
#pragma region publicFunctions

    VulkanRendererNext(GLFWwindow* window);

    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanUp() override;

    virtual void createAndMapMeshBuffers(
        Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices) override;

#pragma endregion publicFunctions

  private:
#pragma region privateFunctions

    void initVulkan();
    void initSwapChain();
    void initCommand();
    void initSyncObjects();
    void initImgui();

    void renderImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView);

    void createSwapChain();
    void recreateSwapChain();
    void destroySwapChain();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) const;
    void destroyBuffer(const AllocatedBuffer& buffer);

    void createGraphicsCommand();
    void createImmediateCommand();
    void submitImmediateCommands(std::function<void(VkCommandBuffer)>&& commandFunc) const;

    void setFrameBufferResized();

#pragma endregion privateFunctions

#pragma region privateVariables

    //  window
    class GLFWwindow* mWindow;

    //  frame
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    //  vulkan stuffs
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice;

    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;

    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;
    std::optional<uint32_t> mGraphicsFamilyIndex;
    std::optional<uint32_t> mPresentFamilyIndex;

    VkCommandPool mCommandPool;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers;
    VkCommandPool mImmediateCommandPool;
    VkCommandBuffer mImmediateCommandBuffer;

    //  sync objects
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFences;
    VkFence mImmediateFence;

    //  memory allocation
    VmaAllocator mAllocator;

    //  deletion stack
    Utils::FunctionStack<> mDeletionStack;

    //  multi frame inflight objects
    uint32_t mCurrentFrameId = 0;

    //  explicitly handle window (framebuffer) resize
    bool mFrameBufferResized = false;

#pragma endregion privateVariables
};
} // namespace Bunny::Render
