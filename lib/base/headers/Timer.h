#pragma once

#include <type_traits>
#include <cassert>
#include <chrono>

//  may need to handle cross platform in the future
#include <windows.h>

namespace Bunny::Base
{

template <typename TimeUnit = double>
    requires std::is_floating_point_v<TimeUnit>
class ITTimer
{
    //  get time since start to the current tick in seconds
    virtual TimeUnit getTime() const = 0;
    //  get time since last tick to the current tick in seconds
    virtual TimeUnit getDeltaTime() const = 0;
    virtual void tick() = 0;
};

template <typename TimeUnit = double>
    requires std::is_floating_point_v<TimeUnit>
class BasicTimer : public ITTimer<TimeUnit>
{
  public:
    void start()
    {
        mStartTimePoint = std::chrono::high_resolution_clock::now();
        mLastTimePoint = mStartTimePoint;
    }
    virtual void tick() override
    {
        const auto currentTimePoint = std::chrono::high_resolution_clock::now();
        mDeltaTime = currentTimePoint - mLastTimePoint;
        mTime = currentTimePoint - mStartTimePoint;
        mLastTimePoint = currentTimePoint;
    }
    virtual TimeUnit getTime() const override { return mTime.count(); }
    virtual TimeUnit getDeltaTime() const override { return mDeltaTime.count(); }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTimePoint;
    std::chrono::time_point<std::chrono::high_resolution_clock> mLastTimePoint;
    std::chrono::duration<TimeUnit> mTime{0};
    std::chrono::duration<TimeUnit> mDeltaTime{0};
};

} // namespace Bunny::Base