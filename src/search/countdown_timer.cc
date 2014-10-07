#include "countdown_timer.h"

#include <limits>

using namespace std;

CountdownTimer::CountdownTimer(double max_time)
    : max_time(max_time) {
}

CountdownTimer::~CountdownTimer() {
}

bool CountdownTimer::is_expired() const {
    return max_time != numeric_limits<double>::infinity() && timer() >= max_time;
}

ostream &operator<<(ostream &os, const CountdownTimer &cd_timer) {
    os << cd_timer.timer;
    return os;
}
