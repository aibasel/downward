#include "abstraction.h"
#include "option_parser.h"
#include "plugin.h"
#include "priority_queue.h"
#include "shrink_fh.h"

#include <cassert>
#include <limits>
#include <map>
#include <vector>

using namespace std;


ShrinkFH::ShrinkFH(const Options& opts)
    :f_start(HighLow(opts.get_enum("highlow_f"))),
     h_start(HighLow(opts.get_enum("highlow_h"))){   
}

ShrinkFH::ShrinkFH(HighLow fs, HighLow hs)
    :f_start(fs),
     h_start(hs) {
}

static void compute_abstraction(
    vector<vector<AbstractStateRef> > &buckets, int target_size, vector<
        slist<AbstractStateRef> > &collapsed_groups) {
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


void ShrinkFH::shrink(Abstraction &abs, int threshold, bool force) {
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
    const int max_vector_size = 50;
    if(abs.max_h > max_vector_size || abs.max_f > max_vector_size) {
        ordered_buckets_use_map(abs, false, buckets);
    } else {
        ordered_buckets_use_vector(abs, false, buckets);
    }

    compute_abstraction(buckets, threshold, collapsed_groups);
    assert(collapsed_groups.size() <= threshold);

    abs.apply_abstraction(collapsed_groups);
    cout << "size of abstraction after shrink: " << abs.size()
         << ", Threshold: " << threshold << endl;
    assert(abs.size() <= threshold || threshold == 1);

}

//TODO: find way to decrease code duplication 
//in the ordered_buckets_use_* methods
void ShrinkFH::ordered_buckets_use_map(
    const Abstraction &abs,
    bool all_in_same_bucket,
    vector<Bucket> &buckets) {
    map<int, map<int, Bucket > > states_by_f_and_h;
    for (AbstractStateRef state = 0; state < abs.num_states; state++) {
        int g = abs.init_distances[state];
        int h = abs.goal_distances[state];
        if (g == QUITE_A_LOT || h == QUITE_A_LOT)
            continue;

        int f = g + h;

        if (all_in_same_bucket) {
            // Put all into the same bucket.
            f = h = 0;
        }

        states_by_f_and_h[f][h].push_back(state);
    }

    if(f_start == HIGH) {
        map<int, map<int, Bucket > >::reverse_iterator f_it;
        for(f_it = states_by_f_and_h.rbegin(); 
            f_it != states_by_f_and_h.rend();
            f_it++) {
            if(h_start == HIGH) {
                map<int, Bucket>::reverse_iterator h_it;
                for(h_it = f_it->second.rbegin();
                    h_it != f_it->second.rend();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    if (!bucket.empty()) { //should always be true
                        buckets.push_back(Bucket());
                        buckets.back().swap(bucket);
                    }
                }
            } else {
                map<int, Bucket>::iterator h_it;
                for(h_it = f_it->second.begin();
                    h_it != f_it->second.end();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    if (!bucket.empty()) { //should always be true
                        buckets.push_back(Bucket());
                        buckets.back().swap(bucket);
                    }
                }
            }
        }
    } else { //if f_start == LOW
        map<int, map<int, Bucket > >::iterator f_it;
        for(f_it = states_by_f_and_h.begin(); 
            f_it != states_by_f_and_h.end();
            f_it++) {
            if(h_start == HIGH) {
                map<int, Bucket>::reverse_iterator h_it;
                for(h_it = f_it->second.rbegin();
                    h_it != f_it->second.rend();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    if (!bucket.empty()) { //should always be true
                        buckets.push_back(Bucket());
                        buckets.back().swap(bucket);
                    }
                }
            } else {
                map<int, Bucket>::iterator h_it;
                for(h_it = f_it->second.begin();
                    h_it != f_it->second.end();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    if (!bucket.empty()) { //should always be true
                        buckets.push_back(Bucket());
                        buckets.back().swap(bucket);
                    }
                }
            }
        }
    }                       
}

void ShrinkFH::ordered_buckets_use_vector(
    const Abstraction &abs,
    bool all_in_same_bucket,
    vector<Bucket> &buckets) {
    vector<vector<Bucket > > states_by_f_and_h;
    states_by_f_and_h.resize(abs.max_f + 1);
    for (int f = 0; f <= abs.max_f; f++)
        states_by_f_and_h[f].resize(min(f, abs.max_h) + 1);
    for (AbstractStateRef state = 0; state < abs.num_states; state++) {
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

    int f_init = (f_start == HIGH ? abs.max_f : 0);
    int f_end = (f_start == LOW ? 0 : abs.max_f);
    int f_incr = (f_init > f_end ? -1 : 1);
    for (int f = f_init; f != f_end; f += f_incr){
        int h_init = (h_start == HIGH ? states_by_f_and_h[f].size() - 1 : 0);
        int h_end = (h_start == LOW ? 0 : states_by_f_and_h[f].size() - 1);
        int h_incr = (h_init > h_end ? -1 : 1);
        for (int h = h_init; h != h_end; h += h_incr) {
            Bucket &bucket = states_by_f_and_h[f][h];
            if (!bucket.empty()) {
                buckets.push_back(Bucket());
                buckets.back().swap(bucket);
            }
        }
    }

}

bool ShrinkFH::has_memory_limit() {
    return true;
}

bool ShrinkFH::is_bisimulation() {
    return false;
}

bool ShrinkFH::is_dfp() {
    return false;
}

string ShrinkFH::description() {
    string descr = string(f_start == HIGH ? "high" : "low") + " f/"
        + (h_start == HIGH ? "high" : "low") + " h";
    return descr;
}

static ShrinkStrategy *_parse(OptionParser &parser){
    vector<string> high_low;
    high_low.push_back("HIGH");
    high_low.push_back("LOW");
    parser.add_enum_option("highlow_f", high_low, 
                           "", "start with high or low f values");
    parser.add_enum_option("highlow_h", high_low, 
                           "", "start with high or low h values");
    Options opts = parser.parse();

    if(!parser.dry_run())
        return new ShrinkFH(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_fh", _parse);
