add_executable(EngineNext)

target_sources(EngineNext
    PUBLIC
        main.cpp
)

target_link_libraries(EngineNext PRIVATE glfw)
target_link_libraries(EngineNext PRIVATE VulkanRenderer)
