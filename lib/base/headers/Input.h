#pragma once

class GLFWwindow;

namespace Bunny::Base
{
    class Window;

    class InputManager
    {
        public:
            void setupWithWindow(const Window& window);

        private:

            static InputManager* msKeyReceiverInstance;
            static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    };
} // namespace Bunny::Base
