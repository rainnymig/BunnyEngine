add_executable(playground)

target_sources(playground PUBLIC main.cpp)

target_link_libraries(playground PRIVATE glm fmt::fmt)