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


unsigned int RandomNumberGenerator::next32_old() {
    return rng();
}


int RandomNumberGenerator::next31_old() {
    return static_cast<int>(next32_old() >> 1);
}


double RandomNumberGenerator::get_double_old() {
    unsigned int a = next32_old() >> 5, b = next32_old() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}


int RandomNumberGenerator::get_int_old(int bound) {
    unsigned int value;
    do {
        value = next31_old();
    } while (value + static_cast<unsigned int>(bound) >= 0x80000000UL);
    // Just using modulo doesn't lead to uniform distribution. This does.
    return static_cast<int>(value % bound);
}
