#include "rng_options.h"

#include "../options/option_parser.h"

#include <cassert>

using namespace std;

namespace utils {
void add_rng_options(options::OptionParser &parser) {
    parser.add_option<int>(
        "random_seed",
        "choose a random seed: \n"
        "-1 means use the global rng object with its seed, specified via the "
        "global option --random-seed\n"
        "[0..infinity) means create an own rng instance with the given seed",
        "-1",
        options::Bounds("-1", "infinity"));
}

shared_ptr<RandomNumberGenerator> parse_rng_from_options(
    const options::Options &options) {
    assert(options.contains("random_seed"));
    int seed = options.get<int>("random_seed");
    if (seed == -1) {
        return rng();
    } else {
        return make_shared<RandomNumberGenerator>(seed);
    }
}
}
