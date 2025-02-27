#include "Input.h"

#include "Window.h"

#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <functional>
#include <cassert>

namespace Bunny::Base
{
    InputManager* InputManager::msKeyReceiverInstance = nullptr;

    void InputManager::setupWithWindow(const Window& window)
    {
        assert(msKeyReceiverInstance == nullptr);

        InputManager::msKeyReceiverInstance = this;

        GLFWwindow* glfwWindow = window.getRawGlfwWindow();

        glfwSetKeyCallback(glfwWindow, &InputManager::onKeyCallback);
    }

    void InputManager::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        const char* keyName = glfwGetKeyName(key, scancode);
        if (action == GLFW_PRESS)
        {
            fmt::print("Key pressed, {}\n", keyName);
        }
        else if (action == GLFW_RELEASE)
        {
            fmt::print("Key released, {}\n", keyName);
        }
    }

} // namespace Bunny::Base
