add_executable(EngineNext)

target_sources(EngineNext
    PUBLIC
        src/main.cpp
        src/Config.h
        src/Config.cpp
        src/World.h
        src/World.cpp
)

target_link_libraries(EngineNext PRIVATE Base fmt::fmt VulkanRenderer inicpp EnTT::EnTT imgui fastgltf::fastgltf glm)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_RIGHT_HANDED)
