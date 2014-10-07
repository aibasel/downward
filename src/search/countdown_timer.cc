#include "countdown_timer.h"

#include <limits>
#include <ostream>

using namespace std;

CountdownTimer::CountdownTimer(double max_time)
    : max_time(max_time) {
}

CountdownTimer::~CountdownTimer() {
}

double CountdownTimer::get_elapsed_time() const {
    return timer();
}

bool CountdownTimer::is_expired() const {
    return max_time != numeric_limits<double>::infinity() && timer() >= max_time;
}

ostream &operator<<(ostream &os, const CountdownTimer &cd_timer) {
    double value = cd_timer.get_elapsed_time();
    if (value < 0 && value > -1e-10)
        value = 0.0;  // We sometimes get inaccuracies from god knows where.
    if (value < 1e-10)
        value = 0.0;  // Don't care about such small values.
    os << value << "s";
    return os;
}
