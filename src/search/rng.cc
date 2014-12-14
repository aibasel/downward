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
    uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(rng);
}

int RandomNumberGenerator::operator()(int bound) {
    assert(bound > 0);
    uniform_int_distribution<int> distribution(0, bound - 1);
    return distribution(rng);
}
