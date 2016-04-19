#include "rng.h"

#include "system.h"

#include "../options/option_parser.h"

#include <cassert>
#include <chrono>

using namespace std;

namespace utils {
/*
  Ideally, one would use true randomness here from std::random_device. However,
  there exist platforms where this returns non-random data, which is condoned by
  the standard. On these platforms one would need to additionally seed with time
  and process ID (PID), and therefore generally seeding with time and PID only
  is probably good enough.
*/
RandomNumberGenerator::RandomNumberGenerator() {
    unsigned int secs = static_cast<unsigned int>(
        chrono::system_clock::now().time_since_epoch().count());
    seed(secs + get_process_id());
}

RandomNumberGenerator::RandomNumberGenerator(int seed_) {
    seed(seed_);
}

RandomNumberGenerator::~RandomNumberGenerator() {
}

void RandomNumberGenerator::seed(int seed) {
    rng_.seed(seed);
}

void RandomNumberGenerator::add_rng_options(options::OptionParser &parser) {
    parser.add_option<int>(
        "random_seed",
        "choose a random seed: \n"
        "-1 means use the global rng object with its seed, specified via the "
        "global option --random-seed\n"
        "[0..infinity) means create an own rng instance with the given seed",
        "-1",
        options::Bounds("-1", "infinity"));
}

shared_ptr<RandomNumberGenerator> RandomNumberGenerator::parse_rng_from_options(
    const options::Options &options) {
    assert(options.contains("random_seed"));
    int seed = options.get<int>("random_seed");
    if (seed == -1) {
        return rng();
    } else {
        return make_shared<RandomNumberGenerator>(seed);
    }
}

shared_ptr<RandomNumberGenerator> RandomNumberGenerator::rng() {
    // Use an arbitrary default seed.
    static shared_ptr<RandomNumberGenerator> rng =
        make_shared<RandomNumberGenerator>(2011);
    return rng;
}
}
