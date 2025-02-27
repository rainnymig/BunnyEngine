#include "Config.h"

#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"

#include <imgui.h>
#include <fmt/core.h>
#include <inicpp.h>
#include <entt/entt.hpp>

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

    float accumulatedTime = 0;
    constexpr float interval = 0.5f;
    uint32_t accumulatedFrames = 0;
    float fps = 0;
    while(true)
    {
        timer.tick();

        accumulatedTime += timer.getDeltaTime();
        accumulatedFrames++;
        if (accumulatedTime > interval)
        {
            fps = accumulatedFrames/accumulatedTime;
            accumulatedFrames = 0;
            accumulatedTime = 0;
        }

        if(window.processWindowEvent())
        {
            break;
        }

        renderer.beginRenderFrame();

        ImGui::Begin("Hello!");
        ImGui::Text("Hi there!");
        ImGui::End();


        ImGui::Begin("Game Stats");
        ImGui::Text(fmt::format("FPS: {}", fps).c_str());
        ImGui::End();
    
        renderer.finishRenderFrame();

    }

    renderer.cleanup();
    renderResources.cleanup();

    window.destroyAndTerminate();

    return 0;
}