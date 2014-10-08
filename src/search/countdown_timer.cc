#include "countdown_timer.h"

#include <limits>

using namespace std;

CountdownTimer::CountdownTimer(double max_time)
    : max_time(max_time) {
}

CountdownTimer::~CountdownTimer() {
}

bool CountdownTimer::is_expired() const {
    /*
      We avoid querying the timer when it cannot expire so that we get cleaner
      output from "strace" (which otherwise reports the "times" system call
      millions of times.
    */
    return max_time != numeric_limits<double>::infinity() && timer() >= max_time;
}

ostream &operator<<(ostream &os, const CountdownTimer &cd_timer) {
    os << cd_timer.timer;
    return os;
}
