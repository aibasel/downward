#include "shrink_fh.h"

ShrinkFH::ShrinkFH(const Options& opts)
    :high_f(opts.get<bool>("high_f")),
     high_h(opts.get<bool>("high_h")){   
}

void ShrinkFH::shrink(Abstraction &abs, bool force, int threshold){
    assert(threshold >= 1);
    assert(abs.is_solvable());
    if (abs.size() > threshold)
        cout << "shrink by " << (abs.size() - threshold) << " nodes" << " (from "
             << abs.size() << " to " << threshold << ")" << endl;
    else if (force)
        cout << "shrink forced: prune unreachable/irrelevant states" << endl;
    else
        return;

    vector<slist<AbstractStateRef> > collapsed_groups;

    vector<Bucket > buckets;
    vector<vector<Bucket > > states_by_f_and_h;
    partition_setup(abs, states_by_f_and_h, false);
    for (int f = (high_f ? max_f : 0); 
         f != (high_f ? -1 : max_f + 1);
         (high_f ? --f : ++f)){
        for (int h = (high_h ? states_by_f_and_h[f].size() - 1 : 0); 
             h != (high_h ? -1 : states_by_f_and_h[f].size() + 1);
             (high_h ? --h : ++h)){
            Bucket &bucket = states_by_f_and_h[f][h];
            if (!bucket.empty()) {
                buckets.push_back(Bucket());
                buckets.back().swap(bucket);
            }
        }
    }
    compute_abstraction(buckets, threshold, collapsed_groups);
    assert(collapsed_groups.size() <= threshold);

    abs.apply_abstraction(collapsed_groups);
    cout << "size of abstraction after shrink: " << abs.size() << ", Threshold: "
         << threshold << endl;
    assert(abs.size() <= threshold || threshold == 1);

}

void compute_abstraction(
    vector<vector<AbstractStateRef> > &buckets, int target_size, vector<
        slist<AbstractStateRef> > &collapsed_groups) const {
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

void partition_setup(const Abstraction &abs, vector<vector<Bucket > > &states_by_f_and_h, bool all_in_same_bucket) {
    states_by_f_and_h.resize(abs.max_f + 1);
    for (int f = 0; f <= abs.max_f; f++)
        states_by_f_and_h[f].resize(min(f, abs.max_h) + 1);
    for (AbstractStateRef state = 0; state < num_states; state++) {
        int g = abs.init_distances[state];
        int h = abs.goal_distances[state];
        if (g == QUITE_A_LOT || h == QUITE_A_LOT)
            continue;

        int f = g + h;

        if (all_in_same_bucket) {
            // Put all into the same bucket.
            f = h = 0;
        }

        assert(f >= 0 && f < states_by_f_and_h.size());
        assert(h >= 0 && h < states_by_f_and_h[f].size());
        states_by_f_and_h[f][h].push_back(state);
    }
}

static ShrinkStrategy *_parse(OptionParser &parser){
    parser.add_option<bool>("high_f", "start with high f values");
    parser.add_option<bool>("high_h", "start with high h values");
    Options opts = parser.parse();

    if(!parser.dry_run())
        return new ShrinkFH(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_fh", _parse);
