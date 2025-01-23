cmake_minimum_required(VERSION 3.26)

include(FetchContent)

project(BunnyEngine
        VERSION "0.0.1"
        DESCRIPTION "A cute little graphics engine like a bunny.")

set (CMAKE_CXX_STANDARD 20)

# Vulkan
find_package(Vulkan REQUIRED)

# Third Parties
# GLFW
add_subdirectory("./third-party/glfw")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
# GLM
add_subdirectory("./third-party/glm")
# Stb Image
add_subdirectory("./third-party/stb_image")
# Tiny Obj Loader
add_subdirectory("./third-party/tiny_obj_loader")
# Vk-Bootstrap
FetchContent_Declare(
    fetch_vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        v1.3.301 #suggest using a tag so the library doesn't update whenever new commits are pushed to a branch
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)
# Vulkan memory allocator
add_subdirectory("./third-party/vma")
# fmt
add_subdirectory("./third-party/fmt")
# imgui
add_library(imgui)
target_sources(imgui 
    PRIVATE
        "third-party/imgui/imgui.h"
        "third-party/imgui/imgui.cpp"
        "third-party/imgui/imgui_demo.cpp"
        "third-party/imgui/imgui_draw.cpp"
        "third-party/imgui/imgui_widgets.cpp"
        "third-party/imgui/imgui_tables.cpp" 
        "third-party/imgui/backends/imgui_impl_vulkan.cpp"
        "third-party/imgui/backends/imgui_impl_glfw.cpp"
)
target_include_directories(imgui PUBLIC "third-party/imgui/" "third-party/imgui/backends")
target_link_libraries(imgui PUBLIC Vulkan::Vulkan glfw)
# fastgltf
add_subdirectory("./third-party/fastgltf")

# Bunny stuff
add_subdirectory("./lib/base")
add_subdirectory("./lib/time")
add_subdirectory("./lib/rendering/renderer")
add_subdirectory("./lib/rendering/vulkan")
add_subdirectory("./lib/physics")
add_subdirectory("./lib/task-system")
add_subdirectory("./engine")
add_subdirectory("./playground")
