#include "VulkanRendererNext.h"

#include "Helper.h"
#include "Vertex.h"
#include "Error.h"
#include "ErrorCheck.h"
#include "PipelineBuilder.h"
#include "Mesh.h"

#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <VkBootstrap.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <chrono>
#include <iostream>
#include <algorithm>
#include <set>
#include <array>
#include <cassert>

namespace Bunny::Render
{
VulkanRendererNext::VulkanRendererNext(GLFWwindow* window) : mWindow(window)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto renderer = reinterpret_cast<VulkanRendererNext*>(glfwGetWindowUserPointer(window));
        renderer->setFrameBufferResized();
    });
}

void VulkanRendererNext::initialize()
{
    initVulkan();
    initSwapChain();
    initCommand();
    initSyncObjects();
    initDepthResources();
    initImgui();

    initAssetBank();

    //  these are needed by initMaterials so run first
    //  need clean up later
    mScene.setDevice(mDevice);
    mScene.setRenderer(this);
    mScene.buildDescriptorSetLayout();
    mScene.initDescriptorAllocator();

    initMaterials();
    initScene();
}

void VulkanRendererNext::render()
{
    //  wait for previous frame to finish rendering
    vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrameId], VK_TRUE, UINT64_MAX);

    //  wait for image in the swap chain to be available before acquiring it
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrameId], VK_NULL_HANDLE, &imageIndex);

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
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrameId]);

    VkCommandBuffer cmdBuf = mCommandBuffers[mCurrentFrameId];
    //  reset and command buffer
    vkResetCommandBuffer(cmdBuf, 0);

    //  begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VK_HARD_CHECK(vkBeginCommandBuffer(cmdBuf, &beginInfo))

    transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
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

    //  run render passes
    mScene.render(cmdBuf, glm::mat4(1.0f));

    //  end render
    vkCmdEndRendering(cmdBuf);

    //  run render ui

    transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    //  end command buffer
    VK_HARD_CHECK(vkEndCommandBuffer(cmdBuf))

    //  submit the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrameId]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrameId];
    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrameId]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VK_HARD_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrameId]))

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
    result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

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

void VulkanRendererNext::cleanUp()
{
    //  wait for rendering operations to finish before cleaning up
    vkDeviceWaitIdle(mDevice);

    mDeletionStack.Flush();
}

void VulkanRendererNext::createAndMapMeshBuffers(
    Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(NormalVertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    //  create vertex and index buffer
    mesh->mVertexBuffer =
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    mesh->mIndexBuffer =
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    //  create staging buffer, map the vertex and index data into it,
    //  then transfer to the actual vertex and index buffer
    AllocatedBuffer stagingBuffer = createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    memcpy(stagingBuffer.mAllocationInfo.pMappedData, vertices.data(), vertexBufferSize);
    memcpy((char*)stagingBuffer.mAllocationInfo.pMappedData + vertexBufferSize, indices.data(), indexBufferSize);

    submitImmediateCommands([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy vertexCopy{0};
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertexBufferSize;

        vkCmdCopyBuffer(commandBuffer, stagingBuffer.mBuffer, mesh->mVertexBuffer.mBuffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{0};
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertexBufferSize;
        indexCopy.size = indexBufferSize;

        vkCmdCopyBuffer(commandBuffer, stagingBuffer.mBuffer, mesh->mIndexBuffer.mBuffer, 1, &indexCopy);
    });

    destroyBuffer(stagingBuffer);
}

void VulkanRendererNext::initVulkan()
{
    vkb::InstanceBuilder builder;

    //  create VkInstance
    //  make the vulkan instance, with basic debug features
    auto instanceBuildResult = builder
                                   .set_app_name("BunnyEngine")
#ifdef _DEBUG
                                   .request_validation_layers(true)
                                   .enable_validation_layers(true)
                                   .use_default_debug_messenger()
#else
                                   .request_validation_layers(false)
                                   .enable_validation_layers(false)
#endif
                                   .require_api_version(1, 3, 0)
                                   .build();

    vkb::Instance vkbInstance = instanceBuildResult.value();
    mInstance = vkbInstance.instance;
#ifdef _DEBUG
    mDebugMessenger = vkbInstance.debug_messenger;
#endif

    mDeletionStack.AddFunction([this]() { vkDestroyInstance(mInstance, nullptr); });
#ifdef _DEBUG
    mDeletionStack.AddFunction([this]() { vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger); });
#endif

    //  creat VkSurface
    if (const auto result = glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface); result != VK_SUCCESS)
    {
        PRINT_AND_ABORT("failed to create window surface!");
    }
    mDeletionStack.AddFunction([this]() { vkDestroySurfaceKHR(mInstance, mSurface, nullptr); });

    //  Select VkPhysicalDevice
    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.runtimeDescriptorArray = true;

    VkPhysicalDeviceFeatures featureBasic{};
    featureBasic.samplerAnisotropy = true;

    vkb::PhysicalDeviceSelector selector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice = selector.set_minimum_version(1, 3)
                                                .set_required_features_13(features13)
                                                .set_required_features_12(features12)
                                                .set_required_features(featureBasic)
                                                .set_surface(mSurface)
                                                .select()
                                                .value();

    //  create logical VkDevice
    vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();

    mPhysicalDevice = vkbPhysicalDevice.physical_device;
    mDevice = vkbDevice.device;

    mDeletionStack.AddFunction([this]() { vkDestroyDevice(mDevice, nullptr); });

    //  get queue family
    mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    mGraphicsFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    mPresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mPresentFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    //  initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mPhysicalDevice;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = mInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionStack.AddFunction([this]() { vmaDestroyAllocator(mAllocator); });
}

void VulkanRendererNext::initSwapChain()
{
    createSwapChain();
    mDeletionStack.AddFunction([this]() { destroySwapChain(); });
}

void VulkanRendererNext::initCommand()
{
    createGraphicsCommand();
    createImmediateCommand();
}

void VulkanRendererNext::initSyncObjects()
{
    //  probably the simplest create infos!!
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //  initialize as signaled to not block the rendering of first frame

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
        {
            PRINT_AND_ABORT("failed to create semaphores!");
        }
    }

    mDeletionStack.AddFunction([this]() {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
        }
    });

    VK_HARD_CHECK(vkCreateFence(mDevice, &fenceInfo, nullptr, &mImmediateFence));

    mDeletionStack.AddFunction([this]() { vkDestroyFence(mDevice, mImmediateFence, nullptr); });
}

void VulkanRendererNext::initDepthResources()
{
    createDepthResource();
    mDeletionStack.AddFunction([this]() { destroyDepthResource(); });
}

void VulkanRendererNext::initImgui()
{
    //  create descriptor pool just for imgui
    VkDescriptorPool imguiDescPool;
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    VK_HARD_CHECK(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &imguiDescPool));

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //  enable keyboard and gamepad controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(mWindow, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = mInstance;
    initInfo.PhysicalDevice = mPhysicalDevice;
    initInfo.Device = mDevice;
    initInfo.QueueFamily = mGraphicsFamilyIndex.value();
    initInfo.Queue = mGraphicsQueue;
    initInfo.DescriptorPool = imguiDescPool;
    initInfo.MinImageCount = mSwapChainImages.size();
    initInfo.ImageCount = mSwapChainImages.size();
    initInfo.UseDynamicRendering = true;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &mSwapChainImageFormat;
    ImGui_ImplVulkan_Init(&initInfo);
    ImGui_ImplVulkan_CreateFontsTexture();

    mDeletionStack.AddFunction([this, imguiDescPool]() { vkDestroyDescriptorPool(mDevice, imguiDescPool, nullptr); });
    mDeletionStack.AddFunction([]() { ImGui_ImplVulkan_Shutdown(); });
}

void VulkanRendererNext::initAssetBank()
{
    mMeshAssetBank = std::make_unique<MeshAssetsBank>(this);
    mDeletionStack.AddFunction([this]() { mMeshAssetBank->destroyBank(); });
}

void VulkanRendererNext::initScene()
{
    SceneInitializer::makeExampleScene(this, &mScene, mMeshAssetBank.get());
    mScene.initBuffers();

    mDeletionStack.AddFunction([this]() { mScene.destroyScene(); });
}

void VulkanRendererNext::initMaterials()
{
    BasicBlinnPhongMaterial::Builder builder;

    builder.setColorAttachmentFormat(mSwapChainImageFormat);
    builder.setDepthFormat(mDepthImage.mFormat);
    builder.setSceneDescriptorSetLayout(mScene.getSceneDescSetLayout());
    builder.setObjectDescriptorSetLayout(mScene.getObjectDescSetLayout());

    VkPushConstantRange pushConstRange;
    pushConstRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstRange.size = sizeof(ObjectData);
    pushConstRange.offset = 0;
    std::array<VkPushConstantRange, 1> ranges{pushConstRange};
    builder.setPushConstantRanges(ranges);

    mBlinnPhongMaterial = builder.buildMaterial(mDevice);

    mDeletionStack.AddFunction([this]() { mBlinnPhongMaterial->cleanupPipeline(); });
}

void VulkanRendererNext::renderImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo colorAttachment =
        makeColorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = makeRenderingInfo(mSwapChainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(commandBuffer, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRendering(commandBuffer);
}

void VulkanRendererNext::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysicalDevice, mSurface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    vkb::SwapchainBuilder swapchainBuilder{mPhysicalDevice, mDevice, mSurface};

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
}

void VulkanRendererNext::recreateSwapChain()
{
    //  handle window minimization
    //  if window is minimized, pause and wait
    int width = 0, height = 0;
    glfwGetFramebufferSize(mWindow, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(mWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(mDevice);

    destroyDepthResource();
    destroySwapChain();

    createSwapChain();
    createDepthResource();
}

void VulkanRendererNext::destroySwapChain()
{
    // vkDestroyImageView(mDevice, mDepthImageView, nullptr);
    // vkDestroyImage(mDevice, mDepthImage, nullptr);
    // vkFreeMemory(mDevice, mDepthImageMemory, nullptr);

    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
}

void VulkanRendererNext::createDepthResource()
{
    VkFormat depthFormat = findDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        PRINT_AND_ABORT("Can not find suitable depth image format");
    }

    mDepthImage = createImage({mSwapChainExtent.width, mSwapChainExtent.height, 1}, depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

    //  transition the depth image layout
    submitImmediateCommands([this](VkCommandBuffer cmdBuf) {
        transitionImageLayout(cmdBuf, mDepthImage.mImage, mDepthImage.mFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    });
}

void VulkanRendererNext::destroyDepthResource()
{
    vkDestroyImageView(mDevice, mDepthImage.mImageView, nullptr);
    vmaDestroyImage(mAllocator, mDepthImage.mImage, mDepthImage.mAllocation);
}

SwapChainSupportDetails VulkanRendererNext::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
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

VkSurfaceFormatKHR VulkanRendererNext::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const
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

VkPresentModeKHR VulkanRendererNext::chooseSwapPresentMode(
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

VkExtent2D VulkanRendererNext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width;
        int height;
        glfwGetFramebufferSize(mWindow, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

AllocatedBuffer VulkanRendererNext::createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
    VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage) const
{
    AllocatedBuffer newBuffer;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = size,
        .usage = bufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo vmaInfo{
        .flags = vmaCreateFlags,
        .usage = vmaUsage,
    };

    VK_HARD_CHECK(vmaCreateBuffer(
        mAllocator, &bufferInfo, &vmaInfo, &newBuffer.mBuffer, &newBuffer.mAllocation, &newBuffer.mAllocationInfo));

    return newBuffer;
}

void VulkanRendererNext::destroyBuffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(mAllocator, buffer.mBuffer, buffer.mAllocation);
}

uint32_t VulkanRendererNext::getCurrentFrameId() const
{
    return mCurrentFrameId;
}

BasicBlinnPhongMaterial* VulkanRendererNext::getMaterial() const
{
    return mBlinnPhongMaterial.get();
}

AllocatedImage VulkanRendererNext::createImage(
    VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkImageLayout layout)
{
    AllocatedImage newImage;
    newImage.mFormat = format;
    newImage.mExtent = size;

    VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.extent = size;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = format;
    imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCreateInfo.initialLayout = layout;
    imgCreateInfo.usage = usage;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.pNext = nullptr;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.priority = 1;

    VK_HARD_CHECK(
        vmaCreateImage(mAllocator, &imgCreateInfo, &allocCreateInfo, &newImage.mImage, &newImage.mAllocation, nullptr));

    VkImageViewCreateInfo viewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.pNext = nullptr;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.image = newImage.mImage;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    VK_HARD_CHECK(vkCreateImageView(mDevice, &viewCreateInfo, nullptr, &newImage.mImageView));

    return newImage;
}

AllocatedImage VulkanRendererNext::createImageAndMapData(
    void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags)
{
    return AllocatedImage();
}

VkFormat VulkanRendererNext::findSupportedFormat(
    std::span<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat VulkanRendererNext::findDepthFormat() const
{
    std::array<VkFormat, 3> candidates{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    return findSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanRendererNext::createGraphicsCommand()
{
    assert(mGraphicsFamilyIndex.has_value() && mPresentFamilyIndex.has_value());
    //  create command pool
    VkCommandPoolCreateInfo poolInfo =
        makeCommandPoolCreateInfo(mGraphicsFamilyIndex.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_HARD_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool));

    mDeletionStack.AddFunction([this]() { vkDestroyCommandPool(mDevice, mCommandPool, nullptr); });

    //  create command buffer
    VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(mCommandPool, MAX_FRAMES_IN_FLIGHT);
    VK_HARD_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()));
}

void VulkanRendererNext::createImmediateCommand()
{
    //  create command pool
    VkCommandPoolCreateInfo poolInfo =
        makeCommandPoolCreateInfo(mGraphicsFamilyIndex.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_HARD_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mImmediateCommandPool));

    mDeletionStack.AddFunction([this]() { vkDestroyCommandPool(mDevice, mImmediateCommandPool, nullptr); });

    //  create command buffer
    VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(mImmediateCommandPool, 1);
    VK_HARD_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, &mImmediateCommandBuffer));
}

void VulkanRendererNext::submitImmediateCommands(std::function<void(VkCommandBuffer)>&& commandFunc) const
{
    VK_HARD_CHECK(vkResetFences(mDevice, 1, &mImmediateFence));
    VK_HARD_CHECK(vkResetCommandBuffer(mImmediateCommandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_HARD_CHECK(vkBeginCommandBuffer(mImmediateCommandBuffer, &beginInfo));

    commandFunc(mImmediateCommandBuffer);

    VK_HARD_CHECK(vkEndCommandBuffer(mImmediateCommandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mImmediateCommandBuffer;

    VK_HARD_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mImmediateFence));
    VK_HARD_CHECK(vkWaitForFences(mDevice, 1, &mImmediateFence, true, 9999999999));
}

void VulkanRendererNext::setFrameBufferResized()
{
    mFrameBufferResized = true;
}

} // namespace Bunny::Render
