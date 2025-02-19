#include "Timer.h"

#include <fmt/core.h>
#include <thread>
#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    Bunny::Utils::BasicTimer timer;
    timer.start();

    fmt::print("time now is {}, delta time is {}\n", timer.getTime(), timer.getDeltaTime());

    std::this_thread::sleep_for(16ms);
    timer.tick();
    fmt::print("time now is {}, delta time is {}\n", timer.getTime(), timer.getDeltaTime());

    std::this_thread::sleep_for(10ms);
    timer.tick();
    fmt::print("time now is {}, delta time is {}\n", timer.getTime(), timer.getDeltaTime());

    return 0;
}