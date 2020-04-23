#include <ctime>
#include <functional>
#include <iostream>

#include <unistd.h>

#include "system.h"

using namespace std;
using namespace Utils;


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


int main(int, char **) {
    // const int NUM_ITERATIONS = 100000000;
    const int NUM_ITERATIONS = 1000000;

    benchmark("nothing", NUM_ITERATIONS, [] () {});
    benchmark("get_peak_memory_in_kb",
              NUM_ITERATIONS,
              [&]() {get_peak_memory_in_kb();});
    benchmark("sbrk",
              NUM_ITERATIONS,
              [&]() {sbrk(0);});
    cout << endl;
    return 0;
}
