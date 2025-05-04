#pragma once

#include <functional>
#include <unordered_map>
#include <string>

class GLFWwindow;

namespace Bunny::Base
{
class Window;

using CallbackHandle = size_t;

class InputManager
{
  public:
    enum class KeyState : int
    {
        Release = 0,
        Press = 1,
        Hold = 2
    };

    using KeyboardCallback = std::function<void(const std::string&, KeyState)>;

    void setupWithWindow(const Window& window);
    CallbackHandle registerKeyboardCallback(KeyboardCallback callback);
    void unregisterKeyboardCallback(CallbackHandle handle);

  private:
    static InputManager* msKeyReceiverInstance;
    static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onMousePositionCallback(GLFWwindow* window, double xpos, double ypos);

    std::unordered_map<CallbackHandle, KeyboardCallback> mKeyboardCallbacks;
    size_t mNextKeyboardCallbackHandle = 1;

    bool mIsMouseActive = true;
};
} // namespace Bunny::Base
