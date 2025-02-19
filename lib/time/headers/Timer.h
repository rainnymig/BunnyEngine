#pragma once

#include <type_traits>
#include <cassert>
#include <chrono>

//  may need to handle cross platform in the future
#include <windows.h>

namespace Bunny::Utils
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
class TWallClockTimer : public ITTimer<TimeUnit>
{
  public:
    TWallClockTimer()
    {
        BOOL result = QueryPerformanceFrequency(&mPerfFreq);
        assert(result && mPerfFreq.QuadPart);

        start();
    }

    virtual TimeUnit getTime() const override;
    virtual TimeUnit getDeltaTime() const override;
    virtual void tick() override;

  private:
    void start();

    LARGE_INTEGER mPerfFreq;
    LARGE_INTEGER mStartCount;
    LARGE_INTEGER mPrevCount;
    LARGE_INTEGER mThisCount;
    TimeUnit mDeltaTime;
};

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline TimeUnit TWallClockTimer<TimeUnit>::getTime() const
{
    LARGE_INTEGER accumulatedCount;
    accumulatedCount.QuadPart = mThisCount.QuadPart - mStartCount.QuadPart;
    return static_cast<TimeUnit>(accumulatedCount.QuadPart) / static_cast<TimeUnit>(mPerfFreq.QuadPart);
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline TimeUnit TWallClockTimer<TimeUnit>::getDeltaTime() const
{
    return mDeltaTime;
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline void TWallClockTimer<TimeUnit>::start()
{
    QueryPerformanceCounter(&mStartCount);
    mPrevCount.QuadPart = mStartCount.QuadPart;
    mThisCount.QuadPart = mStartCount.QuadPart;
    mDeltaTime = 0;
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline void TWallClockTimer<TimeUnit>::tick()
{
    QueryPerformanceCounter(&mThisCount);
    LARGE_INTEGER deltaCount;
    deltaCount.QuadPart = mThisCount.QuadPart - mPrevCount.QuadPart;
    mDeltaTime = static_cast<TimeUnit>(deltaCount.QuadPart) / mPerfFreq.QuadPart;

    mPrevCount = mThisCount;
}

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

} // namespace Bunny::Utils