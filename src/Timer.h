#pragma once

#include <chrono>
#include <type_traits>
#include <assert.h>

class CTimer final
{
public:
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    using time_point = std::chrono::system_clock::time_point;

    inline CTimer();
    inline ~CTimer();

    CTimer(const CTimer& other) = delete;
    CTimer& operator=(const CTimer& rhs) = delete;

    inline bool IsStart();

    inline void Start();
    inline void Stop();
    template <typename TimeUnit>
    inline TimeUnit GetElapsedTime(bool* retVal = nullptr) const;

private:
    time_point mStartTime;
    bool mIsStart;
};

CTimer::CTimer()
    : mIsStart(false)
{

}

CTimer::~CTimer()
{

}

bool CTimer::IsStart()
{
    return mIsStart;
}

void CTimer::Start()
{
    mStartTime = std::chrono::system_clock::now();
    mIsStart = true;
}

void CTimer::Stop()
{
    mIsStart = false;
}

template <typename TimeUnit>
TimeUnit CTimer::GetElapsedTime(bool* retVal/* = nullptr*/) const
{
    static_assert(std::is_same<TimeUnit, microseconds>::value ||
        std::is_same<TimeUnit, milliseconds>::value ||
        std::is_same<TimeUnit, seconds>::value ||
        std::is_same<TimeUnit, minutes>::value ||
        std::is_same<TimeUnit, hours>::value,
        "Invalid TimeUnit!!!");

    if (mIsStart)
    {
        auto endTime = std::chrono::system_clock::now();
        if (retVal)
        {
            *retVal = true;
        }
        return std::chrono::duration_cast<TimeUnit>(endTime - mStartTime);
    }
    else
    {
        if (retVal)
        {
            *retVal = false;
        }
        return TimeUnit(0);
    }
}
