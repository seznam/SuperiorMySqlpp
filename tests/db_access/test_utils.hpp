/*
 * Author: Tomas Nozicka
 */

#pragma once


#include <chrono>
#include <thread>


template<typename F>
inline void backoffSleep(std::chrono::milliseconds max, std::chrono::milliseconds min, F&& f)
{
    using namespace std::chrono_literals;
    static constexpr auto maxSleepTime = 50ms;
    auto startTime = std::chrono::system_clock::now();
    decltype(startTime) now{};
    decltype(now-startTime) elapsed{};

    auto sleepTime = min;

    do
    {
        std::this_thread::sleep_for(sleepTime);

        if (f())
        {
            return;
        }

        sleepTime *= 2;
        if (sleepTime > maxSleepTime)
        {
            sleepTime = maxSleepTime;
        }

        now = std::chrono::system_clock::now();
        elapsed = now - startTime;
    } while (elapsed + sleepTime <= max);

    if (elapsed < max)
    {
        std::this_thread::sleep_for(max - elapsed);
    }
}

template<typename F>
inline void backoffSleep(std::chrono::milliseconds max, F&& f)
{
    return backoffSleep(max, std::chrono::milliseconds{1}, std::forward<F>(f));
}
