#include "shrink_random.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../plugins/plugin.h"

#include <cassert>
#include <memory>

using namespace std;

namespace merge_and_shrink {
ShrinkRandom::ShrinkRandom(const plugins::Options &opts)
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

class ShrinkRandomFeature : public plugins::TypedFeature<ShrinkStrategy, ShrinkRandom> {
public:
    ShrinkRandomFeature() : TypedFeature("shrink_random") {
        document_title("Random");
        document_synopsis("");

        ShrinkBucketBased::add_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<ShrinkRandomFeature> _plugin;
}
