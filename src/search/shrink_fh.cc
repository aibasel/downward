#include "shrink_fh.h"

ShrinkFH::ShrinkFH(const Options& opts)
    :high_f(opts.get<bool>("high_f")),
     high_h(opts.get<bool>("high_h")){   
}

void ShrinkFH::shrink(Abstraction &abs, bool force, int threshold){
    vector<vector<Bucket > > states_by_f_and_h;
    partition_setup(abs, states_by_f_and_h, false);
    zjakfl;
}

inline void partition_setup(const Abstraction &abs, vector<vector<Bucket > > &states_by_f_and_h, bool all_in_same_bucket) {
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
