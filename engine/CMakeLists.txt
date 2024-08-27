add_executable(FullEngine main.cpp)

target_compile_features(FullEngine PRIVATE cxx_std_20)

target_link_libraries(FullEngine PRIVATE VulkanRenderer)
target_link_libraries(FullEngine PRIVATE glfw)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")
