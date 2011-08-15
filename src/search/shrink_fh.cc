#include "raz_abstraction.h"
#include "option_parser.h"
#include "plugin.h"
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

ShrinkFH::~ShrinkFH() {
}

void ShrinkFH::shrink(Abstraction &abs, int threshold, bool force) const {
    if(!must_shrink(abs, threshold, force))
        return;

    vector<Bucket > buckets;
    if(abs.max_f > abs.num_states) {
        ordered_buckets_use_map(abs, buckets);
    } else {
        ordered_buckets_use_vector(abs, buckets);
    }
    
    vector<slist<AbstractStateRef> > collapsed_groups;
    ShrinkBucketBased::compute_abstraction(
        buckets, threshold, collapsed_groups);

    apply(abs, collapsed_groups, threshold);
}


void ShrinkFH::ordered_buckets_use_map(
    const Abstraction &abs,
    vector<Bucket> &buckets) const {
    map<int, map<int, Bucket > > states_by_f_and_h;
    for (AbstractStateRef state = 0; state < abs.num_states; state++) {
        int g = abs.init_distances[state];
        int h = abs.goal_distances[state];
        if (g == QUITE_A_LOT || h == QUITE_A_LOT)
            continue;

        int f = g + h;

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
                    assert (!bucket.empty());
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            } else {
                map<int, Bucket>::iterator h_it;
                for(h_it = f_it->second.begin();
                    h_it != f_it->second.end();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    assert (!bucket.empty());
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
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
                    assert (!bucket.empty());
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            } else {
                map<int, Bucket>::iterator h_it;
                for(h_it = f_it->second.begin();
                    h_it != f_it->second.end();
                    h_it++) {
                    Bucket &bucket = h_it->second;
                    assert (!bucket.empty());
                    buckets.push_back(Bucket());
                    buckets.back().swap(bucket);
                }
            }
        }
    }                       
}

void ShrinkFH::ordered_buckets_use_vector(
    const Abstraction &abs,
    vector<Bucket> &buckets) const {
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

        assert(f >= 0 && f < states_by_f_and_h.size());
        assert(h >= 0 && h < states_by_f_and_h[f].size());
        states_by_f_and_h[f][h].push_back(state);
    }

    int f_init = (f_start == HIGH ? abs.max_f : 0);
    int f_end = (f_start == HIGH ? 0 : abs.max_f);
    int f_incr = (f_init > f_end ? -1 : 1);
    for (int f = f_init; f != f_end + f_incr; f += f_incr){
        int h_init = (h_start == HIGH ? states_by_f_and_h[f].size() - 1 : 0);
        int h_end = (h_start == HIGH ? 0 : states_by_f_and_h[f].size() - 1);
        int h_incr = (h_init > h_end ? -1 : 1);
        for (int h = h_init; h != h_end + h_incr; h += h_incr) {
            Bucket &bucket = states_by_f_and_h[f][h];
            if (!bucket.empty()) {
                buckets.push_back(Bucket());
                buckets.back().swap(bucket);
            }
        }
    }

}

bool ShrinkFH::has_memory_limit() const {
    return true;
}

bool ShrinkFH::is_bisimulation() const {
    return false;
}

bool ShrinkFH::is_dfp() const {
    return false;
}

string ShrinkFH::description() const {
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
