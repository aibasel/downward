#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include "system.h"

#include <ostream>

namespace utils {
class Time {
    double seconds;
public:
    explicit Time(double seconds) : seconds(seconds) {}
    operator double() const {
        return seconds;
    }
};

std::ostream &operator<<(std::ostream &os, const Time &time);

class Timer {
    double last_start_clock;
    double collected_time;
    bool stopped;
#if OPERATING_SYSTEM == WINDOWS
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_ticks;
#endif

    double current_clock() const;
    Time round_value(double value) const;
public:
    Timer();
    ~Timer() = default;
    Time operator()() const;
    Time stop();
    void resume();
    Time reset();
};

std::ostream &operator<<(std::ostream &os, const Timer &timer);

extern Timer g_timer;
}

#endif
