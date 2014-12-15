#include "rng.h"

#include <chrono>
#include <unistd.h>

using namespace std;


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
