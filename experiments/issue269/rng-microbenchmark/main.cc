#include <ctime>
#include <functional>
#include <iostream>
#include <string>

#include "alt_inlined_rng.h"
#include "inlined_rng.h"
#include "old_rng.h"
#include "rng.h"

using namespace std;


void benchmark(const string &desc, int num_calls,
               const function<void ()> &func) {
    cout << "Running " << desc << " " << num_calls << " times:" << flush;
    clock_t start = clock();
    for (int i = 0; i < num_calls; ++i)
        func();
    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    cout << " " << duration << " seconds" << endl;
}


int main(int, char **) {
    const int NUM_ITERATIONS = 100000000;

    const int SEED = 2014;
    OldRandomNumberGenerator old_rng(SEED);
    RandomNumberGenerator new_rng(SEED);
    InlinedRandomNumberGenerator inlined_rng(SEED);
    AltInlinedRandomNumberGenerator alt_inlined_rng(SEED);

    benchmark("nothing", NUM_ITERATIONS, [] (){}
              );
    cout << endl;
    benchmark("random double (old RNG)",
              NUM_ITERATIONS,
              [&]() {old_rng();
              }
              );
    benchmark("random double (new RNG, old distribution)",
              NUM_ITERATIONS,
              [&]() {new_rng.get_double_old();
              }
              );
    benchmark("random double (new RNG)",
              NUM_ITERATIONS,
              [&]() {new_rng();
              }
              );
    benchmark("random double (inlined RNG)",
              NUM_ITERATIONS,
              [&]() {inlined_rng();
              }
              );
    benchmark("random double (alternative inlined RNG)",
              NUM_ITERATIONS,
              [&]() {alt_inlined_rng();
              }
              );
    cout << endl;
    benchmark("random int in 0..999 (old RNG)",
              NUM_ITERATIONS,
              [&]() {old_rng(1000);
              }
              );
    benchmark("random int in 0..999 (new RNG, old distribution)",
              NUM_ITERATIONS,
              [&]() {new_rng.get_int_old(1000);
              }
              );
    benchmark("random int in 0..999 (inlined RNG)",
              NUM_ITERATIONS,
              [&]() {inlined_rng(1000);
              }
              );
    return 0;
}
