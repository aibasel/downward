#include "raz_abstraction.h"
#include "shrink_bucket_based.h"
#include <cassert>
#include <iostream>
#include <vector>

ShrinkBucketBased::ShrinkBucketBased() {
}

ShrinkBucketBased::~ShrinkBucketBased() {
}



void ShrinkBucketBased::compute_abstraction(
    vector<vector<AbstractStateRef> > &buckets, int target_size, 
    vector<slist<AbstractStateRef> > &collapsed_groups) {
    typedef slist<AbstractStateRef> Group;
    bool show_combine_buckets_warning = false;

    assert(collapsed_groups.empty());
    collapsed_groups.reserve(target_size);

    int num_states_to_go = 0;
    for (int bucket_no = 0; bucket_no < buckets.size(); bucket_no++)
        num_states_to_go += buckets[bucket_no].size();

    for (int bucket_no = 0; bucket_no < buckets.size(); bucket_no++) {
        vector<AbstractStateRef> &bucket = buckets[bucket_no];
        int remaining_state_budget = target_size - collapsed_groups.size();
        num_states_to_go -= bucket.size();
        int bucket_budget = remaining_state_budget - num_states_to_go;

        if (bucket_budget >= (int)bucket.size()) {
            // Each state in bucket can become a singleton group.
            // cout << "SHRINK NONE: " << bucket_no << endl;
            for (int i = 0; i < bucket.size(); i++) {
                Group group;
                group.push_front(bucket[i]);
                collapsed_groups.push_back(group);
            }
        } else if (bucket_budget <= 1) {
            // The whole bucket must form one group.
            // cout << "SHRINK ALL: " << bucket_no << endl;
            int remaining_buckets = buckets.size() - bucket_no;
            if (remaining_state_budget >= remaining_buckets) {
                collapsed_groups.push_back(Group());
            } else {
                if (bucket_no == 0)
                    collapsed_groups.push_back(Group());
                if (show_combine_buckets_warning) {
                    show_combine_buckets_warning = false;
                    cout
                    << "ARGH! Very small node limit, must combine buckets."
                    << endl;
                }
            }
            Group &group = collapsed_groups.back();
            group.insert(group.begin(), bucket.begin(), bucket.end());
        } else {
            // Complicated case: must combine until bucket_budget.
            // cout << "SHRINK SOME: " << bucket_no << endl;
            // First create singleton groups.
            vector<Group> groups;
            groups.resize(bucket.size());
            for (int i = 0; i < bucket.size(); i++)
                groups[i].push_front(bucket[i]);
            // Then combine groups until required size is reached.
            assert(bucket_budget >= 2 && bucket_budget < groups.size());
            while (groups.size() > bucket_budget) {
                int pos1 = ((unsigned int)rand()) % groups.size();
                int pos2;
                do {
                    pos2 = ((unsigned int)rand()) % groups.size();
                } while (pos1 == pos2);
                groups[pos1].splice(groups[pos1].begin(), groups[pos2]);
                swap(groups[pos2], groups.back());
                assert(groups.back().empty());
                groups.pop_back();
            }
            // Finally add these groups to the result.
            for (int i = 0; i < groups.size(); i++) {
                collapsed_groups.push_back(Group());
                collapsed_groups.back().swap(groups[i]);
            }
        }
    }
}
