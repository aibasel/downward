#include "timer.h"

#include <ctime>
#include <iomanip>
#include <ostream>

#if OPERATING_SYSTEM == LINUX || OPERATING_SYSTEM == OSX
#include <sys/time.h>
#endif

#if OPERATING_SYSTEM == OSX
#include <mach/mach_time.h>
#endif

using namespace std;

namespace utils {
ostream &operator<<(ostream &os, const Duration &time) {
    os << static_cast<double>(time) << "s";
    return os;
}

static double compute_sanitized_duration(double start_clock, double end_clock) {
    /*
        Sometimes we measure durations that are closer to 0 than should be physically possible
        with measurements on a single CPU. Note that with a CPU frequency less than 10 GHz,
        each clock cycle will take more than 1e-10 seconds. Even worse, these close-to-zero durations
        are sometimes negative. We sanitize them to 0.
    */
    double duration = end_clock - start_clock;
    if (duration > -1e-10 && duration < 1e-10)
        duration = 0.0;
    return duration;
}

#if OPERATING_SYSTEM == OSX
static double get_timebase_ratio() {
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    return static_cast<double>(info.numer) / static_cast<double>(info.denom);
}

void mach_absolute_difference(uint64_t end, uint64_t start, struct timespec *tp) {
    constexpr uint64_t nanoseconds_per_second = 1'000'000'000UL;
    static double timebase_ratio = get_timebase_ratio();

    uint64_t difference = end - start;
    uint64_t elapsed_nanoseconds = static_cast<uint64_t>(difference * timebase_ratio);

    tp->tv_sec = elapsed_nanoseconds / nanoseconds_per_second;
    tp->tv_nsec = elapsed_nanoseconds % nanoseconds_per_second;
}
#endif


Timer::Timer(bool start) {
#if OPERATING_SYSTEM == WINDOWS
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start_ticks);
#endif
    collected_time = 0;
    stopped = !start;
    last_start_clock = start ? current_clock() : 0.;
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

Duration Timer::stop() {
    collected_time = (*this)();
    stopped = true;
    return Duration(collected_time);
}

Duration Timer::operator()() const {
    if (stopped)
        return Duration(collected_time);
    else
        return Duration(collected_time
                        + compute_sanitized_duration(last_start_clock, current_clock()));
}

void Timer::resume() {
    if (stopped) {
        stopped = false;
        last_start_clock = current_clock();
    }
}

Duration Timer::reset() {
    double result = (*this)();
    collected_time = 0;
    last_start_clock = current_clock();
    return Duration(result);
}

ostream &operator<<(ostream &os, const Timer &timer) {
    os << fixed << setprecision(6) << timer();
    return os;
}

Timer g_timer;
}
