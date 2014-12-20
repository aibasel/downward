#include "rng.h"

#include <chrono>
#include <unistd.h>

using namespace std;


/*
  Ideally, one would use true randomness here from std::random_device. However,
  there exist platforms where this returns non-random data, which is condoned by
  the standard. On these platforms one would need to additionally seed with time
  and process ID (PID), and therefore generally seeding with time and PID only
  is probably good enough.
*/
RandomNumberGenerator::RandomNumberGenerator() {
    unsigned int secs = chrono::system_clock::now().time_since_epoch().count();
    seed(secs + getpid());
}

RandomNumberGenerator::RandomNumberGenerator(int seed_) {
    seed(seed_);
}

RandomNumberGenerator::~RandomNumberGenerator() {
}

void RandomNumberGenerator::seed(int seed) {
    rng.seed(seed);
}
