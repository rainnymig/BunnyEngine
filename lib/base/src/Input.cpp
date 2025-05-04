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
    glfwSetCursorPosCallback(glfwWindow, &InputManager::onMousePositionCallback);
}

CallbackHandle InputManager::registerKeyboardCallback(KeyboardCallback callback)
{
    assert(!mKeyboardCallbacks.contains(mNextKeyboardCallbackHandle));
    mKeyboardCallbacks[mNextKeyboardCallbackHandle] = callback;
    return mNextKeyboardCallbackHandle++;
}

void InputManager::unregisterKeyboardCallback(CallbackHandle handle)
{
    if (!mKeyboardCallbacks.contains(mNextKeyboardCallbackHandle))
    {
        return;
    }
    mKeyboardCallbacks.erase(handle);
}

void InputManager::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //  if these static asserts failed, the GLFW action values might have changed
    static_assert(GLFW_RELEASE == static_cast<int>(KeyState::Release));
    static_assert(GLFW_PRESS == static_cast<int>(KeyState::Press));
    static_assert(GLFW_REPEAT == static_cast<int>(KeyState::Hold));

    if (const char* keyName = glfwGetKeyName(key, scancode))
    {
        if (*keyName == 'm' && action == GLFW_PRESS)
        {
            msKeyReceiverInstance->mIsMouseActive = !msKeyReceiverInstance->mIsMouseActive;
            glfwSetInputMode(
                window, GLFW_CURSOR, msKeyReceiverInstance->mIsMouseActive ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }

        for (const auto& callback : msKeyReceiverInstance->mKeyboardCallbacks)
        {
            callback.second(keyName, static_cast<KeyState>(action));
        }
    }
}

void InputManager::onMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
}

} // namespace Bunny::Base
