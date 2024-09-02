cmake_minimum_required(VERSION 3.26)

project(BunnyEngine
        VERSION "0.0.1"
        DESCRIPTION "A cute little graphics engine like a bunny.")

# Third Parties
# GLFW
add_subdirectory("./third-party/glfw")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
# GLM
add_subdirectory("./third-party/glm")

# Bunny stuff
add_subdirectory("./lib/rendering/renderer")
add_subdirectory("./lib/rendering/vulkan")
add_subdirectory("./engine")
