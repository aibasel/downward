#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include "system.h"

#include <ostream>

namespace utils {
class Duration {
    double seconds;
public:
    explicit Duration(double seconds) : seconds(seconds) {}
    operator double() const {
        return seconds;
    }
};

std::ostream &operator<<(std::ostream &os, const Duration &time);

class Timer {
    double last_start_clock;
    double collected_time;
    bool stopped;
#if OPERATING_SYSTEM == WINDOWS
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_ticks;
#endif

    double current_clock() const;
public:
    Timer();
    ~Timer() = default;
    Duration operator()() const;
    Duration stop();
    void resume();
    Duration reset();
};

std::ostream &operator<<(std::ostream &os, const Timer &timer);

extern Timer g_timer;
}

#endif
