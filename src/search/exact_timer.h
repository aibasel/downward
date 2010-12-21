#ifndef EXACT_TIMER_H
#define EXACT_TIMER_H

#include <iosfwd>

class ExactTimer {
    double last_start_clock;
    double collected_time;
    bool stopped;

    double current_clock() const;
public:
    ExactTimer();
    ~ExactTimer();
    double operator()() const;
    double stop();
    void resume();
    double reset();
};

std::ostream &operator<<(std::ostream &os, const ExactTimer &timer);

#endif
