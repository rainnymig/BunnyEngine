add_executable(Playground)

target_sources(Playground PUBLIC main.cpp)

target_link_libraries(Playground PRIVATE glm fmt::fmt Base EnTT::EnTT)