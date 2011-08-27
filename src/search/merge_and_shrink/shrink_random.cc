#include "shrink_random.h"

#include "abstraction.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>

using namespace std;


ShrinkRandom::ShrinkRandom(const Options &opts)
    : ShrinkBucketBased(opts) {
}

ShrinkRandom::~ShrinkRandom() {
}

string ShrinkRandom::name() const {
    return "random";
}

void ShrinkRandom::partition_into_buckets(
    const Abstraction &abs, vector<Bucket> &buckets) const {
    assert(buckets.empty());
    buckets.resize(1);
    Bucket &big_bucket = buckets.back();
    big_bucket.reserve(abs.size());
    for (AbstractStateRef state = 0; state < abs.size(); ++state)
        big_bucket.push_back(state);
    assert(!big_bucket.empty());
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    ShrinkStrategy::add_options_to_parser(parser);
    Options opts = parser.parse();
    ShrinkStrategy::handle_option_defaults(opts);

    if (!parser.dry_run())
        return new ShrinkRandom(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_random", _parse);
