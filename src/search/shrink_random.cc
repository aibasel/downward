#include "shrink_random.h"

#include "option_parser.h"
#include "plugin.h"
#include "raz_abstraction.h"

#include <cassert>

using namespace std;


ShrinkRandom::ShrinkRandom() {
}

ShrinkRandom::~ShrinkRandom() {
}

void ShrinkRandom::partition_into_buckets(
    const Abstraction &abs, vector<Bucket> &buckets) const {
    assert(buckets.empty());
    buckets.resize(1);
    Bucket &big_bucket = buckets.back();
    big_bucket.reserve(abs.num_states);
    for (AbstractStateRef state = 0; state < abs.num_states; ++state)
        big_bucket.push_back(state);
    assert(!big_bucket.empty());
}

string ShrinkRandom::description() const {
    return "random";
}

static ShrinkStrategy *_parse(OptionParser &parser){
    Options opts = parser.parse();

    if(!parser.dry_run())
        return new ShrinkRandom();
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_random", _parse);
