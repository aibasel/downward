#include "option_parser.h"
#include "plugin.h"
#include "raz_abstraction.h"
#include "shrink_random.h"
#include <cassert>

ShrinkRandom::ShrinkRandom() {
}

ShrinkRandom::~ShrinkRandom() {
}

void ShrinkRandom::shrink(Abstraction &abs, int threshold, bool force) const {
    if(!must_shrink(abs, threshold, force))
        return;

    vector<Bucket > buckets;
    Bucket big_bucket;

    for (AbstractStateRef state = 0; state < abs.num_states; state++) {
        big_bucket.push_back(state);
    }
    assert (!big_bucket.empty());
    buckets.push_back(Bucket());
    buckets.back().swap(big_bucket);
    
    vector<slist<AbstractStateRef> > collapsed_groups;
    ShrinkBucketBased::compute_abstraction(
        buckets, threshold, collapsed_groups);

    apply(abs, collapsed_groups, threshold);
}



bool ShrinkRandom::has_memory_limit() const {
    return true;
}

bool ShrinkRandom::is_bisimulation() const {
    return false;
}

bool ShrinkRandom::is_dfp() const {
    return false;
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
