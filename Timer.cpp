#include "Timer.h"

Timer::Timer() {}
Timer::Timer(const Timer& other) {}
Timer::~Timer() {}

bool Timer::Initialize() {
    INT64 frequency {};

    // Get the cycles per second speed for this system.
    QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
    if(frequency == 0) {
        return false;
    }

    // Store it in floating point.
    m_frequency = (float)frequency;

    // Get the initial start time.
    QueryPerformanceCounter((LARGE_INTEGER*)&m_startTime);

    return true;
}

void Timer::Frame() {
    INT64 currentTime;
    INT64 elapsedTicks;

    // Query the current time.
    QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

    // Calculate the difference in time since the last time we queried for the current time.
    elapsedTicks = currentTime - m_startTime;

    // Calculate the frame time.
    m_frameTime = (float)elapsedTicks / m_frequency;

    // Restart the timer.
    m_startTime = currentTime;

    return;
}

float Timer::GetTime() {
    return m_frameTime;
}