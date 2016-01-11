#ifndef UTILS_COUNTDOWN_TIMER_H
#define UTILS_COUNTDOWN_TIMER_H

#include "timer.h"

#include <ostream>

namespace utils {
class CountdownTimer {
    Timer timer;
    double max_time;
public:
    explicit CountdownTimer(double max_time);
    ~CountdownTimer();
    bool is_expired() const;
    double get_elapsed_time() const;
    double get_remaining_time() const;
    friend std::ostream &operator<<(std::ostream &os, const CountdownTimer &cd_timer);
};

std::ostream &operator<<(std::ostream &os, const CountdownTimer &cd_timer);
}

#endif
