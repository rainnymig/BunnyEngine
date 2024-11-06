cmake_minimum_required(VERSION 3.26)

include(FetchContent)

project(BunnyEngine
        VERSION "0.0.1"
        DESCRIPTION "A cute little graphics engine like a bunny.")

set (CMAKE_CXX_STANDARD 20)

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

# Bunny stuff
add_subdirectory("./lib/time")
add_subdirectory("./lib/rendering/renderer")
add_subdirectory("./lib/rendering/vulkan")
add_subdirectory("./lib/physics")
add_subdirectory("./lib/task-system")
add_subdirectory("./engine")
