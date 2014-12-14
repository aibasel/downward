#include "rng.h"

#include <cassert>
#include <chrono>

using namespace std;


RandomNumberGenerator::RandomNumberGenerator() {
    unsigned int secs = chrono::system_clock::now().time_since_epoch().count();
    seed(secs);
}

RandomNumberGenerator::RandomNumberGenerator(int seed_) {
    seed(seed_);
}

void RandomNumberGenerator::seed(int seed) {
    rng.seed(seed);
}

double RandomNumberGenerator::operator()() {
    unsigned int a = next_uint() >> 5, b = next_uint() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}

int RandomNumberGenerator::operator()(int bound) {
    assert(bound > 0);
    uniform_int_distribution<int> distribution(0, bound - 1);
    return distribution(rng);
}
