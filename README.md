# Bunny Engine

### before build

1. install vulkan sdk
2. run `git submodule update --init --recursive` to get glfw as git submodule
3. use cmake to configure and build the solution

### how to run

1. build the solution using cmake
2. run the script `compile_glsl_shader.ps1`, this will compile all shaders and copy them to the executable path (debug target)
3. copy `assets` to executable path (`build/engine-next/Debug`)

### useful git submodule commands
- add new submodule: `git submodule add [remote-repo] [path/for/new/submodule]`
- update a submodule to latest: `git submodule update --remote [submodule]`
- after pulling, grab the new submodules that were added somewhere else: `git submodule update --init --recursive`

### TODOs
[Todos](./Todo.md)

### Resources

Model Information:
* title:	Ship_Steampunk
* source:	https://sketchfab.com/3d-models/ship-steampunk-9f07b3371d6f4807b149aee015f73b3d
* author:	Cyril Limoges Athiel (https://sketchfab.com/athiel)

Model License:
* license type:	CC-BY-4.0 (http://creativecommons.org/licenses/by/4.0/)
* requirements:	Author must be credited. Commercial use is allowed.

### dependencies

[Vulkan SDK](https://vulkan.lunarg.com/sdk/home) needs to be installed for vulkan renderer to work. Once it's installed, cmake will be able to find the package.

[volk](https://github.com/zeux/volk) MIT license

[glfw](https://github.com/glfw/glfw) zlib license

[stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) MIT license

[stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h) MIT license

[tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader) MIT license

[vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) MIT license

[vulkan memory allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) MIT license

[imgui](https://github.com/ocornut/imgui) MIT license

[inifile-cpp](https://github.com/Rookfighter/inifile-cpp) MIT license

[EnTT](https://github.com/skypjack/entt) MIT license

[FastNoise2](https://github.com/Auburn/FastNoise2) MIT license