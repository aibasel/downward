#include <ctime>
#include <functional>
#include <iostream>
#include <string>

#include <sys/times.h>
#include <unistd.h>

using namespace std;


void benchmark(const string &desc, int num_calls,
               const function<void()> &func) {
    cout << "Running " << desc << " " << num_calls << " times:" << flush;
    clock_t start = clock();
    for (int i = 0; i < num_calls; ++i)
        func();
    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    cout << " " << duration << " seconds" << endl;
}


double get_time_with_times() {
    struct tms the_tms;
    times(&the_tms);
    clock_t clocks = the_tms.tms_utime + the_tms.tms_stime;
    return double(clocks) / sysconf(_SC_CLK_TCK);
}

double get_time_with_clock_gettime() {
    timespec tp;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
    return tp.tv_sec + tp.tv_nsec / 1e9;
}


int main(int, char **) {
    const int NUM_ITERATIONS = 10000000;

    benchmark("nothing", NUM_ITERATIONS, [] () {});
    cout << endl;
    benchmark("times()",
              NUM_ITERATIONS,
              [&]() {get_time_with_times();});
    benchmark("clock_gettime()",
              NUM_ITERATIONS,
              [&]() {get_time_with_clock_gettime();});
    return 0;
}
