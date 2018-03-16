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
    Duration get_elapsed_time() const;
    Duration get_remaining_time() const;
};
}

#endif
