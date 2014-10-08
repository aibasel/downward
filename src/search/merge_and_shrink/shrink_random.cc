#include "shrink_random.h"

#include "transition_system.h"

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
    const TransitionSystem &ts, vector<Bucket> &buckets) const {
    assert(buckets.empty());
    buckets.resize(1);
    Bucket &big_bucket = buckets.back();
    big_bucket.reserve(ts.get_size());
    int num_states = ts.get_size();
    for (AbstractStateRef state = 0; state < num_states; ++state)
        big_bucket.push_back(state);
    assert(!big_bucket.empty());
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    parser.document_synopsis("Random", "");
    ShrinkStrategy::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    ShrinkStrategy::handle_option_defaults(opts);

    if (!parser.dry_run())
        return new ShrinkRandom(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_random", _parse);
