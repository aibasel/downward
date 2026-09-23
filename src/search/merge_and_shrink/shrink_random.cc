#include "shrink_random.h"

#include "factored_transition_system.h"
#include "transition_system.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

#include <cassert>
#include <memory>

using namespace std;

namespace merge_and_shrink {
ShrinkRandom::ShrinkRandom(
    const shared_ptr<AbstractTask> &task, int random_seed)
    : ShrinkBucketBased(task, random_seed) {
}

vector<ShrinkBucketBased::Bucket> ShrinkRandom::partition_into_buckets(
    const TransitionSystem &ts, const Distances &) const {
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

class ShrinkRandomFeature
    : public plugins::TypedFeature<TaskIndependentShrinkStrategy> {
public:
    ShrinkRandomFeature() : TypedFeature("shrink_random") {
        document_title("Random");
        document_synopsis(
            "Randomly combining abstract states is discussed as a baseline "
            "shrinking strategy in the following paper:" +
            utils::format_conference_reference(
                {"Malte Helmert", "Patrik Haslum", "Jörg Hoffmann"},
                "Flexible Abstraction Heuristics for Optimal Sequential "
                "Planning",
                "https://cdn.aaai.org/ICAPS/2007/ICAPS07-023.pdf",
                "Proceedings of the 17th International Conference on "
                "Automated Planning and Scheduling (ICAPS 2007)",
                "176-183", "AAAI Press", "2007"));

        add_shrink_bucket_options_to_feature(*this);
    }

    virtual shared_ptr<TaskIndependentShrinkStrategy> create_component(
        const plugins::Options &opts) const override {
        return components::make_auto_task_independent_component<
            ShrinkRandom, ShrinkStrategy>(
            get_shrink_bucket_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<ShrinkRandomFeature> _plugin;
}
