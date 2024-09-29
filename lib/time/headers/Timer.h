#pragma once

#include <type_traits>
#include <cassert>

//  may need to handle cross platform in the future
#include <windows.h>

namespace Bunny::Utils
{

template <typename TimeUnit = double>
    requires std::is_floating_point_v<TimeUnit>
class ITTimer
{
    //  get time since start to the current tick in seconds
    virtual TimeUnit GetTime() const = 0;
    //  get time since last tick to the current tick in seconds
    virtual TimeUnit GetDeltaTime() const = 0;
    virtual void Tick() = 0;
};

template <typename TimeUnit = double>
    requires std::is_floating_point_v<TimeUnit>
class TWallClockTimer : public ITTimer<TimeUnit>
{
  public:
    TWallClockTimer()
    {
        BOOL result = QueryPerformanceFrequency(&mPerfFreq);
        assert(result && mPerfFreq);

        Start();
    }

    virtual TimeUnit GetTime() const override;
    virtual TimeUnit GetDeltaTime() const override;
    virtual void Tick() override;

  private:
    void Start();

    LARGE_INTEGER mPerfFreq;
    LARGE_INTEGER mStartCount;
    LARGE_INTEGER mPrevCount;
    LARGE_INTEGER mThisCount;
    TimeUnit mDeltaTime;
};

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline TimeUnit TWallClockTimer<TimeUnit>::GetTime() const
{
    LARGE_INTEGER accumulatedCount;
    accumulatedCount.QuadPart = mThisCount.QuadPart - mStartCount.QuadPart;
    return static_cast<TimeUnit>(accumulatedCount.QuadPart) / mPerfFreq;
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline TimeUnit TWallClockTimer<TimeUnit>::GetDeltaTime() const
{
    return mDeltaTime;
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline void TWallClockTimer<TimeUnit>::Start()
{
    QueryPerformanceCounter(&mStartCount);
    mPrevCount.QuadPart = mStartCount.QuadPart;
    mThisCount.QuadPart = mStartCount.QuadPart;
    mDeltaTime = 0;
}

template <typename TimeUnit>
    requires std::is_floating_point_v<TimeUnit>
inline void TWallClockTimer<TimeUnit>::Tick()
{
    QueryPerformanceCounter(&mThisCount);
    LARGE_INTEGER deltaCount;
    deltaCount.QuadPart = mThisCount.QuadPart - mPrevCount.QuadPart;
    mDeltaTime = static_cast<TimeUnit>(deltaCount.QuadPart) / mPerfFreq.QuadPart;

    mPrevCount = mThisCount;
}

} // namespace Bunny::Utils