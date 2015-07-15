#include "shrink_bucket_based.h"

#include "../globals.h"
#include "../rng.h"

#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

ShrinkBucketBased::ShrinkBucketBased(const Options &opts)
    : ShrinkStrategy(opts) {
}

ShrinkBucketBased::~ShrinkBucketBased() {
}

void ShrinkBucketBased::compute_abstraction(
    const vector<Bucket> &buckets, int target_size,
    StateEquivalenceRelation &equiv_relation) const {
    bool show_combine_buckets_warning = true;

    assert(equiv_relation.empty());
    equiv_relation.reserve(target_size);

    size_t num_states_to_go = 0;
    for (size_t bucket_no = 0; bucket_no < buckets.size(); ++bucket_no)
        num_states_to_go += buckets[bucket_no].size();

    for (size_t bucket_no = 0; bucket_no < buckets.size(); ++bucket_no) {
        const vector<AbstractStateRef> &bucket = buckets[bucket_no];
        int states_used_up = static_cast<int>(equiv_relation.size());
        int remaining_state_budget = target_size - states_used_up;
        num_states_to_go -= bucket.size();
        int budget_for_this_bucket = remaining_state_budget - num_states_to_go;

        if (budget_for_this_bucket >= static_cast<int>(bucket.size())) {
            // Each state in bucket can become a singleton group.
            for (size_t i = 0; i < bucket.size(); ++i) {
                StateEquivalenceClass group;
                group.push_front(bucket[i]);
                equiv_relation.push_back(group);
            }
        } else if (budget_for_this_bucket <= 1) {
            // The whole bucket must form one group.
            int remaining_buckets = buckets.size() - bucket_no;
            if (remaining_state_budget >= remaining_buckets) {
                equiv_relation.push_back(StateEquivalenceClass());
            } else {
                if (bucket_no == 0)
                    equiv_relation.push_back(StateEquivalenceClass());
                if (show_combine_buckets_warning) {
                    show_combine_buckets_warning = false;
                    cout << "Very small node limit, must combine buckets."
                         << endl;
                }
            }
            StateEquivalenceClass &group = equiv_relation.back();
            group.insert_after(group.before_begin(), bucket.begin(), bucket.end());
        } else {
            // Complicated case: must combine until bucket budget is met.
            // First create singleton groups.
            vector<StateEquivalenceClass> groups(bucket.size());
            for (size_t i = 0; i < bucket.size(); ++i)
                groups[i].push_front(bucket[i]);

            // Then combine groups until required size is reached.
            assert(budget_for_this_bucket >= 2 &&
                   budget_for_this_bucket < static_cast<int>(groups.size()));
            while (static_cast<int>(groups.size()) > budget_for_this_bucket) {
                auto it1 = g_rng.choose(groups);
                auto it2 = it1;
                while (it1 == it2) {
                    it2 = g_rng.choose(groups);
                }
                it1->splice_after(it1->before_begin(), *it2);
                swap(*it2, groups.back());
                assert(groups.back().empty());
                groups.pop_back();
            }

            // Finally add these groups to the result.
            for (size_t i = 0; i < groups.size(); ++i) {
                equiv_relation.push_back(StateEquivalenceClass());
                equiv_relation.back().swap(groups[i]);
            }
        }
    }
}

void ShrinkBucketBased::shrink(const TransitionSystem &ts,
                               int target,
                               StateEquivalenceRelation &equiv_relation) {
    vector<Bucket> buckets;
    partition_into_buckets(ts, buckets);
    compute_abstraction(buckets, target, equiv_relation);
}
