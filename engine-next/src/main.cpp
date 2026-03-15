#include "Config.h"

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "World.h"
#include "WorldLoader.h"
#include "WorldDataTranslator.h"
#include "WorldComponents.h"
#include "WorldSystems.h"
#include "MeshBank.h"
#include "Material.h"
#include "MaterialBank.h"
#include "Vertex.h"
#include "CullingPass.h"
#include "DepthReducePass.h"
#include "GBufferPass.h"
#include "DeferredShadingPass.h"
#include "TextureBank.h"
#include "PbrForwardPass.h"
#include "AccelerationStructureBuilder.h"
#include "RaytracingShadowPass.h"
#include "TexturePreviewPass.h"
#include "SkyPass.h"
#include "FinalOutputPass.h"
#include "ImguiHelper.h"
#include "OceanPass.h"
#include "WaveSpectrumPrePass.h"
#include "WaveSpectrumTransformPass.h"

#include <imgui.h>
#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>
#include <memory>

using namespace Bunny::Engine;
using namespace Bunny::Render;
using Bunny::Base::ImguiHelper;
using Bunny::Base::BasicTimer;

using PbrMaterialParameters = Bunny::Render::PbrMaterialParameters;

int main(void)
{
    fmt::print("Welcome to Bunny Engine!\n");

#ifdef _DEBUG
    fmt::print("This is DEBUG build.\n");
#else
    fmt::print("This is RELEASE build.\n");
#endif

    Config::setup();
    Config::get().loadConfigFile("./assets/config.ini");

    Bunny::Base::Window window;
    window.initialize(Config::get().mWindowWidth, Config::get().mWindowHeight, Config::get().mIsFullScreen,
        Config::get().mWindowName);

    Bunny::Base::InputManager inputManager;
    inputManager.setupWithWindow(window);

    VulkanRenderResources renderResources;

    if (!BUNNY_SUCCESS(renderResources.initialize(&window)))
    {
        PRINT_AND_ABORT("Fail to initialize render resources.")
    }

    VulkanGraphicsRenderer renderer(&renderResources);

    if (!BUNNY_SUCCESS(renderer.initialize(Config::get().mMultiSampleCount)))
    {
        PRINT_AND_ABORT("Fail to initialize graphics renderer.")
    }

    ImguiHelper::setup();

    BasicTimer timer;

    TextureBank textureBank(&renderResources, &renderer);
    MeshBank<NormalVertex> meshBank(&renderResources);
    PbrMaterialBank pbrMaterialBank(&renderResources, &renderer, &textureBank);

    textureBank.initialize();
    pbrMaterialBank.initialize();

    World bunnyWorld;
    WorldLoader worldLoader(&renderResources, &pbrMaterialBank, &meshBank, &textureBank);
    worldLoader.loadPbrTestWorldWithGltfMeshes(Config::get().mModelFilePath, bunnyWorld);

    AccelerationStructureBuilder acceStructBuilder(&renderResources, &renderer);
    acceStructBuilder.buildBottomLevelAccelerationStructures(
        meshBank.getBlasGeometryData(), VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    WaveSpectrumPrePass waveSpectrumPrePass(&renderResources, &renderer);
    WaveSpectrumTransformPass waveTransformPass(
        &renderResources, &renderer, waveSpectrumPrePass.getGridN(), waveSpectrumPrePass.getWidth());
    RaytracingShadowPass rtShadowPass(&renderResources, &renderer, &pbrMaterialBank, &meshBank);
    PbrForwardPass pbrForwardPass(&renderResources, &renderer, &pbrMaterialBank, &meshBank,
        "pbr_culled_instanced_vert.spv", "pbr_forward_frag.spv");
    CullingPass cullingPass(&renderResources, &renderer, &meshBank);
    SkyPass skyPass(&renderResources, &renderer, &textureBank);
    FinalOutputPass finalOutputPass(&renderResources, &renderer, &textureBank);
    DepthReducePass depthReducePass(&renderResources, &renderer);
    TexturePreviewPass texturePreviewPass(&renderResources, &renderer, &pbrMaterialBank, &meshBank, &textureBank);
    OceanPass oceanPass(&renderResources, &renderer, &textureBank, &pbrMaterialBank, &meshBank,
        waveSpectrumPrePass.getWidth(), 512, 4096);

    pbrMaterialBank.recreateMaterialBuffer();

    waveSpectrumPrePass.initializePass();
    waveTransformPass.initializePass();
    rtShadowPass.initializePass();
    pbrForwardPass.initializePass();
    cullingPass.initializePass();
    skyPass.initializePass();
    finalOutputPass.initializePass();
    depthReducePass.initializePass();
    texturePreviewPass.initializePass();
    if (renderResources.getSupportMeshShader())
    {
        oceanPass.initializePass();
    }

    pbrForwardPass.buildDrawCommands();

    WorldRenderDataTranslator worldTranslator(&renderResources, &renderer, &meshBank);
    worldTranslator.initialize();
    worldTranslator.initObjectDataBuffer(&bunnyWorld);

    //  the top level acce struct needs to be updated when the objects move
    acceStructBuilder.buildTopLevelAccelerationStructures(
        worldTranslator.getObjectData(), VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                             VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR);

    rtShadowPass.updateVertIdxBufferData(meshBank.getVertexBufferAddress(), meshBank.getIndexBufferAddress());
    rtShadowPass.linkWorldData(worldTranslator.getPbrLightBuffer(), worldTranslator.getPbrCameraBuffer());
    rtShadowPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    rtShadowPass.linkTopLevelAccelerationStructure(acceStructBuilder.getTopLevelAccelerationStructure().mAcceStruct);

    pbrForwardPass.updateDrawInstanceCounts(worldTranslator.getMeshInstanceCounts());
    pbrForwardPass.linkWorldData(worldTranslator.getPbrLightBuffer(), worldTranslator.getPbrCameraBuffer());
    pbrForwardPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    pbrForwardPass.linkShadowData(rtShadowPass.getOutImageViews());

    cullingPass.linkCullingData(depthReducePass.getDepthHierarchyImages(), depthReducePass.getDepthReduceSampler());
    cullingPass.linkMeshData();
    cullingPass.linkObjectData(worldTranslator.getObjectBuffer(), worldTranslator.getObjectBufferSize());
    cullingPass.linkDrawData(pbrForwardPass.getDrawCommandBuffer(), pbrForwardPass.getDrawCommandBufferSize(),
        pbrForwardPass.getInstanceObjectBuffer(), pbrForwardPass.getInstanceObjectBufferSize());
    cullingPass.setObjectCount(worldTranslator.getObjectCount());
    cullingPass.setDepthImageSizes(depthReducePass.getDepthImageWidth(), depthReducePass.getDepthImageHeight(),
        depthReducePass.getDepthHierarchyLevels());

    skyPass.linkLightData(worldTranslator.getPbrLightBuffer());

    waveTransformPass.updateSpectrumImage(&waveSpectrumPrePass.getSpectrumImage());

    if (renderResources.getSupportMeshShader())
    {
        oceanPass.linkLightAndCameraData(worldTranslator.getPbrLightBuffer(), worldTranslator.getPbrCameraBuffer());
        oceanPass.linkSceneAccelerationStructure(acceStructBuilder.getTopLevelAccelerationStructure().mAcceStruct);
    }

    CameraSystem cameraSystem(&inputManager);

    float accumulatedTime = 0;
    constexpr float interval = 0.5f;
    uint32_t accumulatedFrames = 0;
    float fps = 0;
    uint64_t totalFrames = 0;

    auto showBasicInfo = [&fps]() {
        ImGui::Begin("Game Stats");
        ImGui::Text(fmt::format("FPS: {}", fps).c_str());
        ImGui::Separator();
        ImGui::Text("Movement: W: forward S: backward A: left D: right E: up C: down");
        ImGui::Text("Look: I: down K: up J: left L: right");
        ImGui::End();
    };
    ImguiHelper::get().registerCommand(showBasicInfo);
    auto showCamAndLightControl = [&bunnyWorld, &worldTranslator]() {
        worldTranslator.showImguiControlPanel(&bunnyWorld);
    };
    ImguiHelper::get().registerCommand(showCamAndLightControl);
    auto showTexturePreviewControl = [&texturePreviewPass]() { texturePreviewPass.showImguiControls(); };
    ImguiHelper::get().registerCommand(showTexturePreviewControl);

    IdType spectrumImageDebugId = BUNNY_INVALID_ID;
    bool shouldGenerateSpectrum = true;

    timer.start();
    while (true)
    {
        timer.tick();

        accumulatedTime += timer.getDeltaTime();
        accumulatedFrames++;
        totalFrames++;
        if (accumulatedTime > interval)
        {
            fps = accumulatedFrames / accumulatedTime;
            accumulatedFrames = 0;
            accumulatedTime = 0;
        }

        if (window.processWindowEvent())
        {
            break;
        }

        cameraSystem.update(&bunnyWorld, timer.getDeltaTime());

        //  update object data buffer
        worldTranslator.updateObjectData(&bunnyWorld);

        //  update acceleration structures
        acceStructBuilder.buildTopLevelAccelerationStructures(worldTranslator.getObjectData(),
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR,
            true);

        worldTranslator.updatePbrWorldData(&bunnyWorld);
        pbrMaterialBank.updateMaterialBuffer();
        const auto camComps = bunnyWorld.mEntityRegistry.view<PbrCameraComponent>();
        if (!camComps.empty())
        {
            const auto& cam = bunnyWorld.mEntityRegistry.get<PbrCameraComponent>(camComps.front());
            cullingPass.updateCullingData(cam.mCamera);
            skyPass.updateRenderParams(cam.mCamera, timer.getTime());
            waveTransformPass.updateWaveTime(timer.getTime());
            if (renderResources.getSupportMeshShader())
            {
                oceanPass.updateWorldParams(cam.mCamera.getViewProjMatrix(), timer.getTime(), timer.getDeltaTime());
            }
        }

        texturePreviewPass.updateTextureForPreview();

        //  the drawings begin
        renderer.beginRenderFrame();

        pbrForwardPass.prepareDrawCommandsForFrame();

        if (shouldGenerateSpectrum)
        {
            shouldGenerateSpectrum = false;
            waveSpectrumPrePass.draw();
        }

        waveTransformPass.draw();

        cullingPass.dispatch();
        rtShadowPass.draw();

        pbrForwardPass.updateRenderTarget(renderer.isMultiSampleEnabled() ? &renderer.getMultiSampledColorImage()
                                                                          : &renderer.getColorImageResolved());
        pbrForwardPass.draw();

        if (spectrumImageDebugId == BUNNY_INVALID_ID)
        {
            IdType texId;
            textureBank.addAllocatedTexture(waveTransformPass.getWaveDisplacementImage(), spectrumImageDebugId);
            textureBank.addAllocatedTexture(waveTransformPass.getWaveNormalImage(), texId);
        }

        if (renderResources.getSupportMeshShader())
        {
            oceanPass.updateWaveTextures(
                &waveTransformPass.getWaveDisplacementImage(), &waveTransformPass.getWaveNormalImage());
            oceanPass.updateRenderTarget(renderer.isMultiSampleEnabled() ? &renderer.getMultiSampledColorImage()
                                                                         : &renderer.getColorImageResolved());
            oceanPass.prepareFrameDescriptors();
            oceanPass.draw();
        }

        skyPass.updateFrameData();
        skyPass.draw();

        finalOutputPass.updateInputTextures(&skyPass.getCurrentCloudTexture(), &skyPass.getCurrentFogShadowTexture(),
            &renderer.getColorImageResolved());
        finalOutputPass.draw();

        depthReducePass.dispatch();
        texturePreviewPass.draw();

        renderer.beginImguiFrame();
        ImguiHelper::get().render();
        renderer.finishImguiFrame();

        renderer.finishRenderFrame();
    }

    renderer.waitForRenderFinish();

    cullingPass.cleanup();
    depthReducePass.cleanup();
    finalOutputPass.cleanup();
    skyPass.cleanup();
    oceanPass.cleanup();
    pbrForwardPass.cleanup();
    rtShadowPass.cleanup();
    acceStructBuilder.cleanup();
    worldTranslator.cleanup();
    texturePreviewPass.cleanup();
    waveTransformPass.cleanup();
    waveSpectrumPrePass.cleanup();

    meshBank.cleanup();
    pbrMaterialBank.cleanup();
    textureBank.cleanup();

    renderer.cleanup();
    renderResources.cleanup();

    window.destroyAndTerminate();

    return 0;
}