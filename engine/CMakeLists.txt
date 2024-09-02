set(FULL_ENGINE_SOURCE 
    Engine.h
    Engine.cpp
    main.cpp)

add_executable(FullEngine ${FULL_ENGINE_SOURCE})

target_link_libraries(FullEngine PRIVATE VulkanRenderer)
target_link_libraries(FullEngine PRIVATE glfw)
