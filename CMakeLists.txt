cmake_minimum_required(VERSION 3.26)

project(BunnyEngine
        VERSION "0.0.1"
        DESCRIPTION "A cute little graphics engine like a bunny.")

add_subdirectory("./lib/rendering/vulkan")

add_subdirectory("./engine")