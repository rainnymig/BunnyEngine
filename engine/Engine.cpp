#include "Engine.h"

#include <VulkanRendererFactory.h>

#include <GLFW/glfw3.h>

namespace Bunny
{

void Engine::run()
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
    {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(960, 540, "Bunny Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    std::unique_ptr<Bunny::Render::RendererFactory> factory = std::make_unique<Bunny::Render::VulkanRendererFactory>();
    std::unique_ptr<Bunny::Render::Renderer> renderer = factory->makeRenderer(window);
    renderer->initialize();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();

        renderer->render();
    }

    renderer->cleanUp();

    glfwDestroyWindow(window);
    glfwTerminate();
}
} // namespace Bunny