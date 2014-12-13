#include "rng.h"

#include <ctime>

using namespace std;


RandomNumberGenerator::RandomNumberGenerator() {
    rng.seed(static_cast<int>(time(0)));
}

RandomNumberGenerator::RandomNumberGenerator(int s) {
    rng.seed(s);
}

RandomNumberGenerator::RandomNumberGenerator(
    unsigned int *init_key, int key_length) {
    seed(init_key, key_length);
}

void RandomNumberGenerator::seed(int se) {
    // Seeds should not be zero. Other possible solutions (such as s |= 1)
    // lead to more confusion, because often-used low seeds like 2 and 3 would
    // be identical. This leads to collisions only for rarely used seeds (see
    // note in header file).
    unsigned int s = (static_cast<unsigned int>(se) << 1) + 1;
    rng.seed(s);
}

void RandomNumberGenerator::seed(unsigned int *init_key, int key_length) {
    seed_seq sequence(init_key, init_key + key_length);
    rng.seed(sequence);
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
