# Bunny Engine

### dependencies

[Vulkan SDK](https://vulkan.lunarg.com/sdk/home) needs to be installed for vulkan renderer to work. Once it's installed, cmake will be able to find the package.

[glfw](https://github.com/glfw/glfw) zlib license.

[stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) MIT license.

[tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader) MIT license. 

[vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) MIT license.

[vulkan memory allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) MIT license

[imgui](https://github.com/ocornut/imgui) MIT license

### before build

1. install vulkan sdk
2. run `git submodule update --init --recursive` to get glfw as git submodule
3. use cmake to configure and build the solution