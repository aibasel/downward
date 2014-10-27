#include "exact_timer.h"

#include "system.h"
#include "utilities.h"

#include <ostream>
#include <unistd.h>
using namespace std;


ExactTimer::ExactTimer() {
    last_start_clock = current_clock();
    collected_time = 0;
    stopped = false;
}

ExactTimer::~ExactTimer() {
}

double ExactTimer::current_clock() const {
    return current_system_clock_exact();
}

double ExactTimer::stop() {
    collected_time = (*this)();
    stopped = true;
    return collected_time;
}

double ExactTimer::operator()() const {
    if (stopped)
        return collected_time;
    else
        return collected_time + current_clock() - last_start_clock;
}

void ExactTimer::resume() {
    if (stopped) {
        stopped = false;
        last_start_clock = current_clock();
    }
}

double ExactTimer::reset() {
    double result = (*this)();
    collected_time = 0;
    last_start_clock = current_clock();
    return result;
}

ostream &operator<<(ostream &os, const ExactTimer &timer) {
    double value = timer() / 1e9;
    if (value < 0 && value > -1e-10)
        value = 0.0;  // We sometimes get inaccuracies from God knows where.
    if (value < 1e-10)
        value = 0.0;  // Don't care about such small values.
    os << value << "s";
    return os;
}
