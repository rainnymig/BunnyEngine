#include "Timer.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <fmt/core.h>
#include <chrono>
#include <vector>
#include <string>

struct TestData
{
    int m1;
    float m2;
    std::string m3;
    glm::mat4 mat1;
    glm::vec4 v1;
};

int main()
{
    using namespace std::chrono_literals;

    Bunny::Base::BasicTimer timer;

    const size_t num = 1000000;

    std::vector<TestData> v1(num);
    std::vector<TestData> v2;
    std::vector<TestData> v3;

    timer.start();
    v2.reserve(num);
    std::copy(v1.begin(), v1.end(), std::back_inserter(v2));
    timer.tick();
    fmt::print("delta time is {}\n", timer.getDeltaTime());
    timer.tick();
    v3.reserve(num);
    memcpy(v3.data(), v1.data(), num * sizeof(TestData));
    timer.tick();
    fmt::print("delta time is {}\n", timer.getDeltaTime());

    return 0;
}