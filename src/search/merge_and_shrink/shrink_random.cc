#include "shrink_random.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <memory>

using namespace std;

namespace merge_and_shrink {
ShrinkRandom::ShrinkRandom(const Options &opts)
    : ShrinkBucketBased(opts) {
}

vector<ShrinkBucketBased::Bucket> ShrinkRandom::partition_into_buckets(
    const TransitionSystem &ts,
    const Distances &) const {
    vector<Bucket> buckets;
    buckets.resize(1);
    Bucket &big_bucket = buckets.back();
    big_bucket.reserve(ts.get_size());
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state)
        big_bucket.push_back(state);
    assert(!big_bucket.empty());
    return buckets;
}

string ShrinkRandom::name() const {
    return "random";
}

static shared_ptr<ShrinkStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis("Random", "");
    ShrinkBucketBased::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ShrinkRandom>(opts);
}

static Plugin<ShrinkStrategy> _plugin("shrink_random", _parse);
}
