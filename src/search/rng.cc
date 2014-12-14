#include "rng.h"

#include <ctime>

using namespace std;


RandomNumberGenerator::RandomNumberGenerator() {
    rng.seed(static_cast<int>(time(0)));
}

RandomNumberGenerator::RandomNumberGenerator(int seed) {
    rng.seed(seed);
}

void RandomNumberGenerator::seed(int seed) {
    rng.seed(seed);
}

unsigned int RandomNumberGenerator::next32() {
    return rng();
}

int RandomNumberGenerator::next31() {
    return static_cast<int>(next32() >> 1);
}

double RandomNumberGenerator::next_closed() {
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740991.0);
}

double RandomNumberGenerator::next_half_open() {
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}

double RandomNumberGenerator::next_open() {
    unsigned int a = next32() >> 5, b = next32() >> 6;
    return (0.5 + a * 67108864.0 + b) * (1.0 / 9007199254740991.0);
}

int RandomNumberGenerator::next(int bound) {
    unsigned int value;
    do {
        value = next31();
    } while (value + static_cast<unsigned int>(bound) >= 0x80000000UL);
    // Just using modulo doesn't lead to uniform distribution. This does.
    return static_cast<int>(value % bound);
}
