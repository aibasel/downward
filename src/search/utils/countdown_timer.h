#ifndef UTILS_COUNTDOWN_TIMER_H
#define UTILS_COUNTDOWN_TIMER_H

#include "timer.h"

namespace utils {
class CountdownTimer {
    Timer timer;
    double max_time;
public:
    explicit CountdownTimer(double max_time);
    ~CountdownTimer();
    bool is_expired() const;
    Time get_elapsed_time() const;
    Time get_remaining_time() const;
};
}

#endif
