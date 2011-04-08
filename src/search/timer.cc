#include "timer.h"

#include <ostream>
#include <sys/times.h>
#include <unistd.h>

using namespace std;

Timer::Timer() {
    last_start_clock = current_clock();
    collected_time = 0;
    stopped = false;
}

Timer::~Timer() {
}

double Timer::current_clock() const {
    struct tms the_tms;
    times(&the_tms);
    clock_t clocks = the_tms.tms_utime + the_tms.tms_stime;
    return double(clocks) / sysconf(_SC_CLK_TCK);
}

double Timer::stop() {
    collected_time = (*this)();
    stopped = true;
    return collected_time;
}

double Timer::operator()() const {
    if (stopped)
        return collected_time;
    else
        return collected_time + current_clock() - last_start_clock;
}

void Timer::resume() {
    if (stopped) {
        stopped = false;
        last_start_clock = current_clock();
    }
}

double Timer::reset() {
    double result = (*this)();
    collected_time = 0;
    last_start_clock = current_clock();
    return result;
}

ostream &operator<<(ostream &os, const Timer &timer) {
    double value = timer();
    if (value < 0 && value > -1e-10)
        value = 0.0;  // We sometimes get inaccuracies from god knows where.
    if (value < 1e-10)
        value = 0.0;  // Don't care about such small values.
    os << value << "s";
    return os;
}
