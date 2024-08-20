add_executable(FullEngine main.cpp)

target_compile_features(FullEngine PRIVATE cxx_std_20)

target_link_libraries(FullEngine PRIVATE VulkanRenderer)