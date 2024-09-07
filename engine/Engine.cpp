#include "Engine.h"

#include <VulkanRenderer.h>
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(960, 540, "Bunny Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    // glClearColor( 0.4f, 0.3f, 0.4f, 0.0f );

    std::unique_ptr<Bunny::Render::RendererFactory> factory = std::make_unique<Bunny::Render::VulkanRendererFactory>();
    std::unique_ptr<Bunny::Render::Renderer> renderer = factory->makeRenderer(window);
    renderer->initialize();
    // renderer->render();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        // glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    renderer->cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
}
} // namespace Bunny