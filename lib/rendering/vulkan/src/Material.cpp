#include "Material.h"

#include "Shader.h"
#include "Error.h"
#include "ErrorCheck.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
Material::~Material()
{
}

BasicBlinnPhongMaterial::BasicBlinnPhongMaterial(Base::BunnyGuard<Builder> guard, VkDevice device) : mDevice(device)
{
}

void BasicBlinnPhongMaterial::cleanup()
{
    cleanupPipeline();
}

void BasicBlinnPhongMaterial::cleanupPipeline()
{
    if (mDevice == nullptr)
    {
        return;
    }

    // mDescriptorAllocator.destroyPools(mDevice);
    vkDestroyPipeline(mDevice, mPipeline.mPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipeline.mPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mSceneDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mObjectDescSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mDrawDescSetLayout, nullptr);
    // vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);

    mDevice = nullptr;
}

MaterialInstance BasicBlinnPhongMaterial::makeInstance()
{
    //  Note: maybe create instance cache instead of recreating every time

    static int instanceIdCounter = 1;

    MaterialInstance newInstance;

    newInstance.mpBaseMaterial = &mPipeline;
    // mDescriptorAllocator.allocate(mDevice, &mDescriptorSetLayout, &newInstance.mDescriptorSet);

    return newInstance;
}

std::unique_ptr<BasicBlinnPhongMaterial> BasicBlinnPhongMaterial::Builder::buildMaterial(VkDevice device) const
{
    assert(device != nullptr);

    std::unique_ptr<BasicBlinnPhongMaterial> material = std::make_unique<BasicBlinnPhongMaterial>(CARROT, device);

    if (!BUNNY_SUCCESS(buildDescriptorSetLayouts(
            device, material->mSceneDescSetLayout, material->mObjectDescSetLayout, material->mDrawDescSetLayout)))
    {
        PRINT_AND_RETURN_VALUE("Fail to build descriptor set layouts for Basic Blinn Phong Material", nullptr);
    }

    if (!BUNNY_SUCCESS(buildPipeline(device, material->mSceneDescSetLayout, material->mObjectDescSetLayout,
            material->mDrawDescSetLayout, material->mPipeline)))
    {
        PRINT_AND_RETURN_VALUE("Fail to build pipeline for Basic Blinn Phong Material", nullptr);
    }

    //  no material descriptor required for now
    //  init descriptor allocator
    // constexpr size_t MAX_SETS = 2; //   should be max frames inflight
    // DescriptorAllocator::PoolSize poolSizes[] = {
    //     {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2},
    // };
    // material->mDescriptorAllocator.Init(device, MAX_SETS, poolSizes);

    return std::move(material);
}

BunnyResult BasicBlinnPhongMaterial::Builder::buildPipeline(VkDevice device, VkDescriptorSetLayout sceneLayout,
    VkDescriptorSetLayout objectLayout, VkDescriptorSetLayout drawLayout, MaterialPipeline& outPipeline) const
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    //  load shader file
    Shader vertexShader(mVertexShaderPath, device);
    Shader fragmentShader(mFragmentShaderPath, device);

    //  build pipeline

    VkDescriptorSetLayout layouts[] = {sceneLayout, objectLayout, drawLayout};

    //  pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout))

    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    GraphicsPipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.setColorAttachmentFormats(std::vector<VkFormat>{mColorFormat});
    builder.setDepthFormat(mDepthFormat);
    builder.setPipelineLayout(pipelineLayout);

    pipeline = builder.build(device);

    if (pipeline == nullptr)
    {
        PRINT_AND_RETURN_VALUE("Can not build Basic Blinn Phong Material pipeline", BUNNY_SAD)
    }

    outPipeline.mPipeline = pipeline;
    outPipeline.mPipelineLayout = pipelineLayout;

    return BUNNY_HAPPY;
}

BunnyResult BasicBlinnPhongMaterial::Builder::buildDescriptorSetLayouts(VkDevice device,
    VkDescriptorSetLayout& outSceneLayout, VkDescriptorSetLayout& outObjectLayout,
    VkDescriptorSetLayout& outDrawLayout) const
{
    //  object data desc sets
    DescriptorLayoutBuilder layoutBuilder;
    VkDescriptorSetLayoutBinding storageBufferBinding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
    layoutBuilder.addBinding(storageBufferBinding);
    outObjectLayout = layoutBuilder.build(device);
    outDrawLayout = layoutBuilder.build(device);

    //  scene data desc sets
    layoutBuilder.clear();
    {
        VkDescriptorSetLayoutBinding uniformBufferBinding{
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
        layoutBuilder.addBinding(uniformBufferBinding);
        uniformBufferBinding.binding = 1;
        uniformBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBuilder.addBinding(uniformBufferBinding);
    }
    outSceneLayout = layoutBuilder.build(device);

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
