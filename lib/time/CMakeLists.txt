add_library(Time)

target_sources(Time 
    PUBLIC
        headers/Timer.h
    PRIVATE
        src/Timer.cpp
)

target_include_directories(Time PUBLIC ./headers)
