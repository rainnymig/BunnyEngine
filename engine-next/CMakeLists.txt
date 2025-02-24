add_executable(EngineNext)

target_sources(EngineNext
    PUBLIC
        src/main.cpp
        src/Config.h
        src/Config.cpp
)

target_link_libraries(EngineNext PRIVATE Base fmt::fmt Time VulkanRenderer inicpp EnTT::EnTT)
