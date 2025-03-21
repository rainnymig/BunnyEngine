#include "Engine.h"

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
    window = glfwCreateWindow(1280, 720, "Bunny Engine", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();

    }


    glfwDestroyWindow(window);
    glfwTerminate();
}
} // namespace Bunny