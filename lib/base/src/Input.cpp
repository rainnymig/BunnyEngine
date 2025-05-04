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

KeyboardCallbackHandle InputManager::registerKeyboardCallback(KeyboardCallback callback)
{
    assert(!mKeyboardCallbacks.contains(mNextKeyboardCallbackHandle));
    mKeyboardCallbacks[mNextKeyboardCallbackHandle] = callback;
    return mNextKeyboardCallbackHandle++;
}

void InputManager::unregisterKeyboardCallback(KeyboardCallbackHandle handle)
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
        for (const auto& callback : msKeyReceiverInstance->mKeyboardCallbacks)
        {
            callback.second(keyName, static_cast<KeyState>(action));
        }
    }
}

} // namespace Bunny::Base
