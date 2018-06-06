#include "shrink_fh.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/collections.h"
#include "../utils/markup.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

using namespace std;

namespace merge_and_shrink {
ShrinkFH::ShrinkFH(const Options &opts)
    : ShrinkBucketBased(opts),
      f_start(HighLow(opts.get_enum("shrink_f"))),
      h_start(HighLow(opts.get_enum("shrink_h"))) {
}

vector<ShrinkBucketBased::Bucket> ShrinkFH::partition_into_buckets(
    const TransitionSystem &ts,
    const Distances &distances) const {
    assert(distances.are_init_distances_computed());
    assert(distances.are_goal_distances_computed());
    int max_h = 0;
    int max_f = 0;
    for (int state = 0; state < ts.get_size(); ++state) {
        int g = distances.get_init_distance(state);
        int h = distances.get_goal_distance(state);
        int f;
        if (g == INF || h == INF) {
            /*
              If not pruning unreachable or irrelevant states, we may have
              states with g- or h-values of infinity, which we need to treat
              manually here to avoid overflow.

              Also note that not using full pruning means that if there is at
              least one dead state, this strategy will always use the
              map-based approach for partitioning. This is important because
              the vector-based approach requires that there are no dead states.
            */
            f = INF;
        } else {
            f = g + h;
        }
        max_h = max(max_h, h);
        max_f = max(max_f, f);
    }
    // Calculate with double to avoid overflow.
    if (static_cast<double>(max_f) * max_f / 2.0 > ts.get_size()) {
        // Use map because an average bucket in the vector structure
        // would contain less than 1 element (roughly).
        return ordered_buckets_use_map(ts, distances);
    } else {
        return ordered_buckets_use_vector(ts, distances, max_f, max_h);
    }
}

// Helper function for ordered_buckets_use_map.
template<class HIterator, class Bucket>
static void collect_h_buckets(
    HIterator begin, HIterator end,
    vector<Bucket> &buckets) {
    for (HIterator iter = begin; iter != end; ++iter) {
        Bucket &bucket = iter->second;
        assert(!bucket.empty());
        buckets.push_back(Bucket());
        buckets.back().swap(bucket);
    }
}

// Helper function for ordered_buckets_use_map.
template<class FHIterator, class Bucket>
static void collect_f_h_buckets(
    FHIterator begin, FHIterator end,
    ShrinkFH::HighLow h_start,
    vector<Bucket> &buckets) {
    for (FHIterator iter = begin; iter != end; ++iter) {
        if (h_start == ShrinkFH::HIGH) {
            collect_h_buckets(iter->second.rbegin(), iter->second.rend(),
                              buckets);
        } else {
            collect_h_buckets(iter->second.begin(), iter->second.end(),
                              buckets);
        }
    }
}

vector<ShrinkBucketBased::Bucket> ShrinkFH::ordered_buckets_use_map(
    const TransitionSystem &ts,
    const Distances &distances) const {
    map<int, map<int, Bucket>> states_by_f_and_h;
    int bucket_count = 0;
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state) {
        int g = distances.get_init_distance(state);
        int h = distances.get_goal_distance(state);
        int f;
        if (g == INF || h == INF) {
            f = INF;
        } else {
            f = g + h;
        }
        Bucket &bucket = states_by_f_and_h[f][h];
        if (bucket.empty())
            ++bucket_count;
        bucket.push_back(state);
    }

    vector<Bucket> buckets;
    buckets.reserve(bucket_count);
    if (f_start == HIGH) {
        collect_f_h_buckets(
            states_by_f_and_h.rbegin(), states_by_f_and_h.rend(),
            h_start, buckets);
    } else {
        collect_f_h_buckets(
            states_by_f_and_h.begin(), states_by_f_and_h.end(),
            h_start, buckets);
    }
    assert(static_cast<int>(buckets.size()) == bucket_count);
    return buckets;
}

vector<ShrinkBucketBased::Bucket> ShrinkFH::ordered_buckets_use_vector(
    const TransitionSystem &ts,
    const Distances &distances,
    int max_f,
    int max_h) const {
    vector<vector<Bucket>> states_by_f_and_h;
    states_by_f_and_h.resize(max_f + 1);
    for (int f = 0; f <= max_f; ++f)
        states_by_f_and_h[f].resize(min(f, max_h) + 1);
    int bucket_count = 0;
    int num_states = ts.get_size();
    for (int state = 0; state < num_states; ++state) {
        int g = distances.get_init_distance(state);
        int h = distances.get_goal_distance(state);
        // If the state is dead, we should use ordered_buckets_use_map instead.
        assert(g != INF && h != INF);
        int f = g + h;
        assert(utils::in_bounds(f, states_by_f_and_h));
        assert(utils::in_bounds(h, states_by_f_and_h[f]));
        Bucket &bucket = states_by_f_and_h[f][h];
        if (bucket.empty())
            ++bucket_count;
        bucket.push_back(state);
    }

    vector<Bucket> buckets;
    buckets.reserve(bucket_count);
    int f_init = (f_start == HIGH ? max_f : 0);
    int f_end = (f_start == HIGH ? 0 : max_f);
    int f_incr = (f_init > f_end ? -1 : 1);
    for (int f = f_init; f != f_end + f_incr; f += f_incr) {
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
    assert(static_cast<int>(buckets.size()) == bucket_count);
    return buckets;
}

string ShrinkFH::name() const {
    return "f-preserving";
}

void ShrinkFH::dump_strategy_specific_options() const {
    cout << "Prefer shrinking high or low f states: "
         << (f_start == HIGH ? "high" : "low") << endl
         << "Prefer shrinking high or low h states: "
         << (h_start == HIGH ? "high" : "low") << endl;
}

static shared_ptr<ShrinkStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "f-preserving shrink strategy",
        "This shrink strategy implements the algorithm described in"
        " the paper:" + utils::format_paper_reference(
            {"Malte Helmert", "Patrik Haslum", "Joerg Hoffmann"},
            "Flexible Abstraction Heuristics for Optimal Sequential Planning",
            "http://ai.cs.unibas.ch/papers/helmert-et-al-icaps2007.pdf",
            "Proceedings of the Seventeenth International Conference on"
            " Automated Planning and Scheduling (ICAPS 2007)",
            "176-183",
            "2007"));
    parser.document_note(
        "shrink_fh()",
        "Combine this with the merge-and-shrink option max_states=N (where N "
        "is a numerical parameter for which sensible values include 1000, "
        "10000, 50000, 100000 and 200000) and the linear merge startegy "
        "cg_goal_level to obtain the variant 'f-preserving shrinking of "
        "transition systems', called called HHH in the IJCAI 2011 paper, see "
        "bisimulation based shrink strategy. "
        "When we last ran experiments on interaction of shrink strategies "
        "with label reduction, this strategy performed best when used with "
        "label reduction before merging (and no label reduction before "
        "shrinking). "
        "We also recommend using full pruning with this shrink strategy, "
        "because both distances from the initial state and to the goal states "
        "must be computed anyway, and because the existence of only one "
        "dead state causes this shrink strategy to always use the map-based "
        "approach for partitioning states rather than the more efficient "
        "vector-based approach.");
    ShrinkBucketBased::add_options_to_parser(parser);
    vector<string> high_low;
    high_low.push_back("HIGH");
    high_low.push_back("LOW");
    parser.add_enum_option(
        "shrink_f", high_low,
        "prefer shrinking states with high or low f values",
        "HIGH");
    parser.add_enum_option(
        "shrink_h", high_low,
        "prefer shrinking states with high or low h values",
        "LOW");
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ShrinkFH>(opts);
}

static PluginShared<ShrinkStrategy> _plugin("shrink_fh", _parse);
}
