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

ShrinkRandom::~ShrinkRandom() {
}

void ShrinkRandom::partition_into_buckets(
    const FactoredTransitionSystem &fts,
    int index,
    vector<Bucket> &buckets) const {
    const TransitionSystem &ts = fts.get_ts(index);
    assert(buckets.empty());
    buckets.resize(1);
    Bucket &big_bucket = buckets.back();
    big_bucket.reserve(ts.get_size());
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state)
        big_bucket.push_back(state);
    assert(!big_bucket.empty());
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

static PluginShared<ShrinkStrategy> _plugin("shrink_random", _parse);
}
