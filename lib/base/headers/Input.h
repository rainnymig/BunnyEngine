#pragma once

#include <functional>
#include <unordered_map>
#include <string>

class GLFWwindow;

namespace Bunny::Base
{
class Window;

using KeyboardCallbackHandle = size_t;

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
    KeyboardCallbackHandle registerKeyboardCallback(KeyboardCallback callback);
    void unregisterKeyboardCallback(KeyboardCallbackHandle handle);

  private:
    static InputManager* msKeyReceiverInstance;
    static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    std::unordered_map<KeyboardCallbackHandle, KeyboardCallback> mKeyboardCallbacks;
    size_t mNextKeyboardCallbackHandle = 1;
};
} // namespace Bunny::Base
