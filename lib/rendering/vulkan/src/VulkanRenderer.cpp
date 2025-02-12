#include "VulkanRenderer.h"

#include "Helper.h"
#include "Vertex.h"
#include "ErrorCheck.h"
#include "PipelineBuilder.h"
#include "Mesh.h"
#include "Shader.h"
#include "Error.h"

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

namespace
{
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

constexpr std::array<const char*, 1> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

// const std::vector<Bunny::Render::BasicVertex> vertices = {
//     {{-0.5f, -0.5f, 0.0f},  {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//     {{0.5f, -0.5f, 0.0f},   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//     {{0.5f, 0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//     {{-0.5f, 0.5f, 0.0f},   {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

//     {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//     {{0.5f, -0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//     {{0.5f, 0.5f, -0.5f},   {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//     {{-0.5f, 0.5f, -0.5f},  {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
// };

// const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

std::vector<Bunny::Render::BasicVertex> modelVertices = {};

std::vector<uint32_t> modelIndecies = {};

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

} // namespace

namespace Bunny::Render
{
void VulkanRenderer::initialize()
{
    initVulkan();
    initSwapChain();
    createDepthResources();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommand();
    createSyncObjects();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorAllocator();
    createDescriptorSets();
    setupDepthResourcesLayout();
    initImgui();
}

void VulkanRenderer::render()
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
        PRINT_AND_RETURN("failed to acquire swap chain image!");
    }

    //  reset fence only after image is successfully acquired
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrameId]);

    //  update uniform buffers
    updateUniformBuffer(mCurrentFrameId);

    //  update imgui
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();

    // ImGui::ShowDemoWindow(&mRenderImguiDemo);

    ImGui::Begin("Hello!");
    ImGui::Text("Hi there!");
    ImGui::End();

    ImGui::Render();

    {
        VkCommandBuffer cmdBuf = mCommandBuffers[mCurrentFrameId];

        //  reset and command buffer
        vkResetCommandBuffer(cmdBuf, 0);

        //  begin command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(cmdBuf, &beginInfo) != VK_SUCCESS)
        {
            PRINT_AND_RETURN("failed to begin recording command buffer!");
        }

        //  bind graphics pipeline
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

        transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkClearValue colorClearValue = {
            .color = {0.0f, 0.0f, 0.0f, 1.0f}
        };
        VkRenderingAttachmentInfo colorAttachment =
            makeColorAttachmentInfo(mSwapChainImageViews[imageIndex], &colorClearValue);
        VkRenderingAttachmentInfo depthAttachment = makeDepthAttachmentInfo(mDepthImageView);

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

        vkCmdBeginRendering(cmdBuf, &renderInfo);

        //  here begins the rendering of objects
        {
            //  bind vertex buffer
            VkBuffer vertexBuffers[] = {mVertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);

            //  bind index buffer
            vkCmdBindIndexBuffer(cmdBuf, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

            //  bind descriptor sets
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                &mDescriptorSets[mCurrentFrameId], 0, nullptr);

            //  draw!!
            vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(modelIndecies.size()), 1, 0, 0, 0);
        }

        vkCmdEndRendering(cmdBuf);

        renderImgui(cmdBuf, mSwapChainImageViews[imageIndex]);

        transitionImageLayout(cmdBuf, mSwapChainImages[imageIndex], mSwapChainImageFormat,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        //  end command buffer
        if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS)
        {
            PRINT_AND_RETURN("failed to record command buffer!");
        }
    }

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

    if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrameId]) != VK_SUCCESS)
    {
        PRINT_AND_RETURN("failed to submit draw command buffer!");
    }

    //  present!
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
        PRINT_AND_RETURN("failed to present swap chain image!");
    }

    mCurrentFrameId = (mCurrentFrameId + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::cleanUp()
{
    //  wait for rendering operations to finish before cleaning up
    vkDeviceWaitIdle(mDevice);

    mDeletionStack.Flush();
}

void VulkanRenderer::setFrameBufferResized()
{
    mFrameBufferResized = true;
}

void VulkanRenderer::createAndMapMeshBuffers(Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(NormalVertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    //  create vertex and index buffer
    mesh->mVertexBuffer = createBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    mesh->mIndexBuffer = createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    //  create staging buffer, map the vertex and index data into it,
    //  then transfer to the actual vertex and index buffer
    AllocatedBuffer stagingBuffer =
        createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

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

void VulkanRenderer::createSurface()
{
    if (const auto result = glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface); result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    mDeletionStack.AddFunction([this]() { vkDestroySurfaceKHR(mInstance, mSurface, nullptr); });
}

void VulkanRenderer::initVulkan()
{
    vkb::InstanceBuilder builder;

    constexpr bool useValidationLayers = true;

    //  create VkInstance
    //  make the vulkan instance, with basic debug features
    auto instanceBuildResult = builder.set_app_name("BunnyEngine")
                                   .request_validation_layers(useValidationLayers)
                                   .enable_validation_layers(true)
                                   .use_default_debug_messenger()
                                   .require_api_version(1, 3, 0)
                                   .build();

    vkb::Instance vkbInstance = instanceBuildResult.value();
    mInstance = vkbInstance.instance;
    mDebugMessenger = vkbInstance.debug_messenger;

    mDeletionStack.AddFunction([this]() { vkDestroyInstance(mInstance, nullptr); });
    mDeletionStack.AddFunction([this]() { DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr); });

    //  creat VkSurface
    createSurface();

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
    mQueueFamilyIndices.graphicsFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    mPresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
    mQueueFamilyIndices.presentFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

    //  initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mPhysicalDevice;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = mInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionStack.AddFunction([this]() { vmaDestroyAllocator(mAllocator); });
}

void VulkanRenderer::initSwapChain()
{
    createSwapChain();
    mDeletionStack.AddFunction([this]() { cleanUpSwapChain(); });
}

void VulkanRenderer::createSwapChain()
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

void VulkanRenderer::createImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        mSwapChainImageViews[i] =
            createImageView(mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void VulkanRenderer::createGraphicsPipeline()
{
    //  load shader from hardcoded path for now
    Shader vertexShader("./basic_vert.spv", mDevice);
    Shader fragmentShader("./basic_frag.spv", mDevice);

    //  pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;                  // Optional
    pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;          // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;       // Optional

    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    //  vertex info
    auto bindingDescription = getBindingDescription<BasicVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = BasicVertex::getAttributeDescriptions();

    PipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.setColorAttachmentFormat(mSwapChainImageFormat);
    builder.setDepthFormat(mDepthImageFormat);
    builder.setPipelineLayout(mPipelineLayout);

    mGraphicsPipeline = builder.build(mDevice);

    mDeletionStack.AddFunction([this]() {
        vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
    });
}

void VulkanRenderer::createCommand()
{
    createGraphicsCommand();
    createImmediateCommand();
}

void VulkanRenderer::createGraphicsCommand()
{
    assert(mQueueFamilyIndices.isComplete());
    //  create command pool
    VkCommandPoolCreateInfo poolInfo = makeCommandPoolCreateInfo(
        mQueueFamilyIndices.graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_HARD_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool));

    mDeletionStack.AddFunction([this]() { vkDestroyCommandPool(mDevice, mCommandPool, nullptr); });

    //  create command buffer
    VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(mCommandPool, MAX_FRAMES_IN_FLIGHT);
    VK_HARD_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()));
}

void VulkanRenderer::createImmediateCommand()
{
    //  create command pool
    VkCommandPoolCreateInfo poolInfo = makeCommandPoolCreateInfo(
        mQueueFamilyIndices.graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_HARD_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mImmediateCommandPool));

    mDeletionStack.AddFunction([this]() { vkDestroyCommandPool(mDevice, mImmediateCommandPool, nullptr); });

    //  create command buffer
    VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(mImmediateCommandPool, 1);
    VK_HARD_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, &mImmediateCommandBuffer));
}

void VulkanRenderer::createSyncObjects()
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
            throw std::runtime_error("failed to create semaphores!");
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

void VulkanRenderer::recreateSwapChain()
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

    cleanUpSwapChain();

    createSwapChain();
    createDepthResources();
}

void VulkanRenderer::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(modelVertices[0]) * modelVertices.size();

    //  create staging buffer to get the data from host (cpu)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    //  fill the staging buffer
    void* data;
    vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, modelVertices.data(), (size_t)bufferSize);
    vkUnmapMemory(mDevice, stagingBufferMemory);

    //  create vertex buffer whose content is transfered from staging buffer
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mVertexBuffer, mVertexBufferMemory);

    //  copy the content of staging buffer to vertex buffer
    copyBuffer(stagingBuffer, mVertexBuffer, bufferSize);

    //  clean up staging buffer
    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

    mDeletionStack.AddFunction([this]() {
        vkDestroyBuffer(mDevice, mVertexBuffer, nullptr);
        vkFreeMemory(mDevice, mVertexBufferMemory, nullptr);
    });
}

void VulkanRenderer::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(modelIndecies[0]) * modelIndecies.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, modelIndecies.data(), (size_t)bufferSize);
    vkUnmapMemory(mDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mIndexBuffer, mIndexBufferMemory);

    copyBuffer(stagingBuffer, mIndexBuffer, bufferSize);

    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

    mDeletionStack.AddFunction([this]() {
        vkDestroyBuffer(mDevice, mIndexBuffer, nullptr);
        vkFreeMemory(mDevice, mIndexBufferMemory, nullptr);
    });
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    //  create vertex buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    //  find memory requirements and allocate memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }
    vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
}

AllocatedBuffer VulkanRenderer::createBuffer(
    VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage)
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
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memoryUsage,
    };

    VK_HARD_CHECK(vmaCreateBuffer(
        mAllocator, &bufferInfo, &vmaInfo, &newBuffer.mBuffer, &newBuffer.mAllocation, &newBuffer.mAllocationInfo));

    return newBuffer;
}

void VulkanRenderer::createDescriptorSetLayout()
{
    DescriptorLayoutBuilder builder;

    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        builder.AddBinding(uniformBufferLayout);
    }
    {
        VkDescriptorSetLayoutBinding samplerLayout{};
        samplerLayout.binding = 1;
        samplerLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayout.descriptorCount = 1;
        samplerLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayout.pImmutableSamplers = nullptr;
        builder.AddBinding(samplerLayout);
    }

    mDescriptorSetLayout = builder.Build(mDevice);

    mDeletionStack.AddFunction([this]() { vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr); });
}

void VulkanRenderer::createDescriptorAllocator()
{
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 1},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 1}
    };
    mDescriptorAllocator.Init(mDevice, MAX_FRAMES_IN_FLIGHT, poolSizes);

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.DestroyPools(mDevice); });
}

void VulkanRenderer::createDescriptorSets()
{
    //  allocator descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mDescriptorSetLayout);
    mDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    mDescriptorAllocator.Allocate(mDevice, layouts.data(), mDescriptorSets.data(), MAX_FRAMES_IN_FLIGHT);

    //  fill descriptors
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        DescriptorWriter writer;
        writer.WriteBuffer(0, mUniformBuffers[i], sizeof(UniformBufferObject), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.WriteImage(1, mTextureImageView, mTextureSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.UpdateSet(mDevice, mDescriptorSets[i]);
    }
}

void VulkanRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    mUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    mUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    mUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mUniformBuffers[i],
            mUniformBuffersMemory[i]);
        vkMapMemory(mDevice, mUniformBuffersMemory[i], 0, bufferSize, 0, &mUniformBuffersMapped[i]);
    }

    mDeletionStack.AddFunction([this]() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(mDevice, mUniformBuffers[i], nullptr);
            vkFreeMemory(mDevice, mUniformBuffersMemory[i], nullptr);
        }
    });
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj =
        glm::perspective(glm::radians(45.0f), mSwapChainExtent.width / (float)mSwapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; //  flip the clip space z direction to match Vulkan convention (+z forward)
    memcpy(mUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanRenderer::createTextureImage()
{
    //  load image from file
    int texWidth, texHeight, texChannels;
    // stbi_uc* pixels = stbi_load("./huaji.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    //  create staging buffer to copy the loaded image file into
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(mDevice, stagingBufferMemory);

    //  free the loaded image file
    stbi_image_free(pixels);

    //  create image and image memory
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mTextureImage, mTextureImageMemory);

    //  transition the image layout for copying into
    submitImmediateCommands([this](VkCommandBuffer commandBuffer) {
        transitionImageLayout(commandBuffer, mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    });

    //  copy the data from staging buffer to image
    copyBufferToImage(stagingBuffer, mTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    //  transition the image layout for optimal sampling in shader
    submitImmediateCommands([this](VkCommandBuffer commandBuffer) {
        transitionImageLayout(commandBuffer, mTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
    vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

    mDeletionStack.AddFunction([this]() {
        vkDestroyImage(mDevice, mTextureImage, nullptr);
        vkFreeMemory(mDevice, mTextureImageMemory, nullptr);
    });
}

void VulkanRenderer::createTextureImageView()
{
    mTextureImageView = createImageView(mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    mDeletionStack.AddFunction([this]() { vkDestroyImageView(mDevice, mTextureImageView, nullptr); });
}

void VulkanRenderer::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mTextureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }

    mDeletionStack.AddFunction([this]() { vkDestroySampler(mDevice, mTextureSampler, nullptr); });
}

void VulkanRenderer::createDepthResources()
{
    mDepthImageFormat = findDepthFormat();
    createImage(mSwapChainExtent.width, mSwapChainExtent.height, mDepthImageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage,
        mDepthImageMemory);
    mDepthImageView = createImageView(mDepthImage, mDepthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::setupDepthResourcesLayout()
{
    submitImmediateCommands([this](VkCommandBuffer cmdBuf) {
        transitionImageLayout(cmdBuf, mDepthImage, mDepthImageFormat, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    });
}

void VulkanRenderer::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            BasicVertex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            modelVertices.push_back(vertex);
            modelIndecies.push_back(modelIndecies.size());
        }
    }
}

void VulkanRenderer::initImgui()
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
    initInfo.QueueFamily = mQueueFamilyIndices.graphicsFamily.value();
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

void VulkanRenderer::renderImgui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
{
    VkRenderingAttachmentInfo colorAttachment =
        makeColorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
    VkRenderingInfo renderInfo = makeRenderingInfo(mSwapChainExtent, &colorAttachment, nullptr);

    vkCmdBeginRendering(commandBuffer, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRendering(commandBuffer);
}

void VulkanRenderer::destroyBuffer(const AllocatedBuffer& buffer)
{
    vmaDestroyBuffer(mAllocator, buffer.mBuffer, buffer.mAllocation);
}

void VulkanRenderer::cleanUpSwapChain()
{
    vkDestroyImageView(mDevice, mDepthImageView, nullptr);
    vkDestroyImage(mDevice, mDepthImage, nullptr);
    vkFreeMemory(mDevice, mDepthImageMemory, nullptr);

    // for (auto framebuffer : mSwapChainFramebuffers)
    // {
    //     vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    // }
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
}

SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
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

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
    if (availableFormats.empty())
    {
        throw std::runtime_error("The swap chain does not support any format");
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

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
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

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
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

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
    submitImmediateCommands([srcBuffer, dstBuffer, size](VkCommandBuffer commandBuffer) {
        //  copy
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const
{
    //  create the image for render
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(mDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    //  create and bind memory for image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(mDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(mDevice, image, imageMemory, 0);
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
{
    submitImmediateCommands([buffer, image, width, height](VkCommandBuffer commandBuffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    });
}

VkFormat VulkanRenderer::findSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
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

    throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanRenderer::findDepthFormat() const
{
    return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanRenderer::submitImmediateCommands(std::function<void(VkCommandBuffer)>&& commandFunc) const
{
    VK_HARD_CHECK(vkResetFences(mDevice, 1, &mImmediateFence));
    VK_HARD_CHECK(vkResetCommandBuffer(mImmediateCommandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_HARD_CHECK(vkBeginCommandBuffer(mImmediateCommandBuffer, &beginInfo));

    commandFunc(mImmediateCommandBuffer);

    VK_HARD_CHECK(vkEndCommandBuffer(mImmediateCommandBuffer));

    // VkCommandBufferSubmitInfo cmdBufferSubmitInfo = makeCommandBufferSubmitInfo(mImmediateCommandBuffer);
    // VkSubmitInfo2 submitInfo = makeSubmitInfo2(&cmdBufferSubmitInfo, nullptr, nullptr);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mImmediateCommandBuffer;

    VK_HARD_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mImmediateFence));
    VK_HARD_CHECK(vkWaitForFences(mDevice, 1, &mImmediateFence, true, 9999999999));
}

VulkanRenderer::VulkanRenderer(GLFWwindow* window) : mWindow(window)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        renderer->setFrameBufferResized();
    });
}

} // namespace Bunny::Render
