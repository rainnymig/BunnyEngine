add_executable(NoiseGenerator)

target_sources(NoiseGenerator PUBLIC main.cpp)

target_link_libraries(NoiseGenerator PRIVATE FastNoise StbImage)