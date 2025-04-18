#include "Config.h"

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "World.h"
#include "MeshBank.h"
#include "Material.h"
#include "MaterialBank.h"
#include "Vertex.h"
#include "ForwardPass.h"

#include <imgui.h>
#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>
#include <memory>

using namespace Bunny::Engine;

int main(void)
{
    fmt::print("Welcome to Bunny Engine!\n");

#ifdef _DEBUG
    fmt::print("This is DEBUG build.\n");
#else
    fmt::print("This is RELEASE build.\n");
#endif

    Config config;
    // config.loadConfigFile("./config.ini");

    Bunny::Base::Window window;
    window.initialize(config.mWindowWidth, config.mWindowHeight, config.mIsFullScreen, config.mWindowName);

    Bunny::Base::InputManager inputManager;
    inputManager.setupWithWindow(window);

    Bunny::Render::VulkanRenderResources renderResources;

    if (!BUNNY_SUCCESS(renderResources.initialize(&window)))
    {
        PRINT_AND_ABORT("Fail to initialize render resources.")
    }

    Bunny::Render::VulkanGraphicsRenderer renderer(&renderResources);

    if (!BUNNY_SUCCESS(renderer.initialize()))
    {
        PRINT_AND_ABORT("Fail to initialize graphics renderer.")
    }

    bool shouldRun = true;

    Bunny::Base::BasicTimer timer;

    Bunny::Render::MeshBank<Bunny::Render::NormalVertex> meshBank(&renderResources);
    Bunny::Render::MaterialBank materialBank;

    Bunny::Render::BasicBlinnPhongMaterial::Builder builder;
    builder.setColorAttachmentFormat(renderer.getSwapChainImageFormat());
    builder.setDepthFormat(renderer.getDepthImageFormat());
    std::unique_ptr<Bunny::Render::BasicBlinnPhongMaterial> blinnPhongMaterial =
        builder.buildMaterial(renderResources.getDevice());
    Bunny::Render::MaterialInstance blinnPhongInstance = blinnPhongMaterial->makeInstance();

    Bunny::Render::ForwardPass forwardPass(&renderResources, &renderer, &materialBank, &meshBank);
    forwardPass.initializePass(
        blinnPhongMaterial->getSceneDescSetLayout(), blinnPhongMaterial->getObjectDescSetLayout());

    materialBank.addMaterial(std::move(blinnPhongMaterial));
    materialBank.addMaterialInstance(blinnPhongInstance);

    World bunnyWorld;
    WorldLoader worldLoader(&renderResources, &materialBank, &meshBank);
    worldLoader.loadTestWorld(bunnyWorld);

    WorldRenderDataTranslator worldTranslator(&renderResources, &meshBank, &materialBank);
    worldTranslator.initialize();

    forwardPass.updateSceneData(worldTranslator.getSceneBuffer());
    forwardPass.updateLightData(worldTranslator.getLightBuffer());

    float accumulatedTime = 0;
    constexpr float interval = 0.5f;
    uint32_t accumulatedFrames = 0;
    float fps = 0;
    while (true)
    {
        timer.tick();

        accumulatedTime += timer.getDeltaTime();
        accumulatedFrames++;
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

        bunnyWorld.update(timer.getDeltaTime());

        worldTranslator.translateSceneData(&bunnyWorld);
        worldTranslator.translateObjectData(&bunnyWorld);

        renderer.beginRenderFrame();

        renderer.beginRender();

        for (const Bunny::Render::RenderBatch& batch : worldTranslator.getRenderBatches())
        {
            forwardPass.renderBatch(batch);
        }

        renderer.finishRender();

        renderer.beginImguiFrame();

        ImGui::Begin("Game Stats");
        ImGui::Text(fmt::format("FPS: {}", fps).c_str());
        ImGui::End();

        renderer.finishImguiFrame();

        renderer.finishRenderFrame();
        //
    }

    renderer.waitForRenderFinish();

    worldTranslator.cleanup();
    forwardPass.cleanup();

    meshBank.cleanup();
    materialBank.cleanup();

    renderer.cleanup();
    renderResources.cleanup();

    window.destroyAndTerminate();

    return 0;
}