add_library(TaskSystem)

target_sources(TaskSystem
    PUBLIC
        headers/TaskManager.h
        headers/Task.h
    PRIVATE
        src/TaskManager.cpp
        src/Task.cpp
)

target_include_directories(TaskSystem PUBLIC ./headers)
