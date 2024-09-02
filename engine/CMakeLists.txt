set(FULL_ENGINE_SOURCE 
    Engine.h
    Engine.cpp
    main.cpp)

add_executable(FullEngine ${FULL_ENGINE_SOURCE})

target_compile_features(FullEngine PRIVATE cxx_std_20)

target_link_libraries(FullEngine PRIVATE VulkanRenderer)
target_link_libraries(FullEngine PRIVATE glfw)
