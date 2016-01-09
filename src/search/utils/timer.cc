#include "timer.h"

#include <ctime>
#include <ostream>

#if OPERATING_SYSTEM == LINUX || OPERATING_SYSTEM == OSX
#include <sys/time.h>
#endif

#if OPERATING_SYSTEM == OSX
#include <mach/mach_time.h>
#endif

using namespace std;

namespace utils {
#if OPERATING_SYSTEM == OSX
void mach_absolute_difference(uint64_t end, uint64_t start, struct timespec *tp) {
    uint64_t difference = end - start;
    static mach_timebase_info_data_t info = {
        0, 0
    };

    if (info.denom == 0)
        mach_timebase_info(&info);

    uint64_t elapsednano = difference * (info.numer / info.denom);

    tp->tv_sec = elapsednano * 1e-9;
    tp->tv_nsec = elapsednano - (tp->tv_sec * 1e9);
}
#endif


Timer::Timer() {
#if OPERATING_SYSTEM == WINDOWS
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_ticks);
#endif
    last_start_clock = current_clock();
    collected_time = 0;
    stopped = false;
}

double Timer::current_clock() const {
#if OPERATING_SYSTEM == WINDOWS
    LARGE_INTEGER now_ticks;
    QueryPerformanceCounter(&now_ticks);
    double ticks = static_cast<double>(now_ticks.QuadPart - start_ticks.QuadPart);
    return ticks / frequency.QuadPart;
#else
    timespec tp;
#if OPERATING_SYSTEM == OSX
    static uint64_t start = mach_absolute_time();
    uint64_t end = mach_absolute_time();
    mach_absolute_difference(end, start, &tp);
#else
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
#endif
    return tp.tv_sec + tp.tv_nsec / 1e9;
#endif
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
        value = 0.0;  // We sometimes get inaccuracies from God knows where.
    if (value < 1e-10)
        value = 0.0;  // Don't care about such small values.
    os << value << "s";
    return os;
}

Timer g_timer;
}
