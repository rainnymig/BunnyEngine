#pragma once

#include "Renderer.h"

#include "Descriptor.h"
#include "Fundamentals.h"
#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"
#include "Utils.h"
#include "Vertex.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <array>
#include <functional>
#include <span>

namespace Bunny::Render
{

class Mesh;

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRenderer : public Renderer
{
  public:
    VulkanRenderer(GLFWwindow* window);

    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanUp() override;

    void setFrameBufferResized();

    void createAndMapMeshBuffers(Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices);

  private:
    void createSurface();
    void initVulkan();
    void initSwapChain(); //  create swap chain and register it's clean up function
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    void createCommand();
    void createGraphicsCommand();
    void createImmediateCommand();
    void createSyncObjects();
    void recreateSwapChain();
    void createVertexBuffer();
    void createIndexBuffer();
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
    void createDescriptorSetLayout();
    void createDescriptorAllocator();
    void createDescriptorSets();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createDepthResources();
    void setupDepthResourcesLayout();
    void loadModel();
    void initImgui();
    void renderImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView);
    void destroyBuffer(const AllocatedBuffer& buffer);

    void cleanUpSwapChain();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    VkFormat findSupportedFormat(
        const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;

    void submitImmediateCommands(std::function<void(VkCommandBuffer)>&& commandFunc) const;

    //  window
    class GLFWwindow* mWindow;

    //  vulkan rendering objects
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice;
    QueueFamilyIndices mQueueFamilyIndices;
    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    VkDescriptorSetLayout mDescriptorSetLayout;
    DescriptorAllocator mDescriptorAllocator;
    std::vector<VkDescriptorSet> mDescriptorSets;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    VkCommandPool mCommandPool;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers;

    //  immediate submit command
    VkCommandPool mImmediateCommandPool;
    VkCommandBuffer mImmediateCommandBuffer;
    VkFence mImmediateFence;

    //  deletion stack
    Utils::FunctionStack<> mDeletionStack;

    //  memory allocation
    VmaAllocator mAllocator;

    //  depth stencil
    VkImage mDepthImage;
    VkDeviceMemory mDepthImageMemory;
    VkImageView mDepthImageView;
    VkFormat mDepthImageFormat;

    //  rendering buffers
    VkBuffer mVertexBuffer;
    VkDeviceMemory mVertexBufferMemory;
    VkBuffer mIndexBuffer;
    VkDeviceMemory mIndexBufferMemory;

    //  uniform buffers
    //  multiple buffers for multiple frames in-flight
    std::vector<VkBuffer> mUniformBuffers;
    std::vector<VkDeviceMemory> mUniformBuffersMemory;
    std::vector<void*> mUniformBuffersMapped;

    //  image texture
    VkImage mTextureImage;
    VkDeviceMemory mTextureImageMemory;
    VkImageView mTextureImageView;
    VkSampler mTextureSampler;

    //  vulkan sync objects
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFences;

    //  multi frame inflight objects
    uint32_t mCurrentFrameId = 0;

    //  explicitly handle window (framebuffer) resize
    bool mFrameBufferResized = false;

    //  render imgui demo window
    // bool mRenderImguiDemo = true;
};
} // namespace Bunny::Render