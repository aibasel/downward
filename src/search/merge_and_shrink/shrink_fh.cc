#include "shrink_fh.h"

#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

using namespace std;


ShrinkFH::ShrinkFH(const Options &opts)
    : ShrinkBucketBased(opts),
      f_start(HighLow(opts.get_enum("shrink_f"))),
      h_start(HighLow(opts.get_enum("shrink_h"))) {
}

ShrinkFH::~ShrinkFH() {
}

void ShrinkFH::partition_into_buckets(
    const TransitionSystem &ts, vector<Bucket> &buckets) const {
    assert(buckets.empty());
    int max_f = ts.get_max_f();
    // Calculate with double to avoid overflow.
    if (static_cast<double>(max_f) * max_f / 2.0 > ts.get_size()) {
        // Use map because an average bucket in the vector structure
        // would contain less than 1 element (roughly).
        ordered_buckets_use_map(ts, buckets);
    } else {
        ordered_buckets_use_vector(ts, buckets);
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

void ShrinkFH::ordered_buckets_use_map(
    const TransitionSystem &ts,
    vector<Bucket> &buckets) const {
    map<int, map<int, Bucket > > states_by_f_and_h;
    int bucket_count = 0;
    int num_states = ts.get_size();
    for (AbstractStateRef state = 0; state < num_states; ++state) {
        int g = ts.get_init_distance(state);
        int h = ts.get_goal_distance(state);
        if (g != INF && h != INF) {
            int f = g + h;
            Bucket &bucket = states_by_f_and_h[f][h];
            if (bucket.empty())
                ++bucket_count;
            bucket.push_back(state);
        }
    }

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
}

void ShrinkFH::ordered_buckets_use_vector(
    const TransitionSystem &ts,
    vector<Bucket> &buckets) const {
    vector<vector<Bucket > > states_by_f_and_h;
    states_by_f_and_h.resize(ts.get_max_f() + 1);
    for (int f = 0; f <= ts.get_max_f(); ++f)
        states_by_f_and_h[f].resize(min(f, ts.get_max_h()) + 1);
    int bucket_count = 0;
    int num_states = ts.get_size();
    for (AbstractStateRef state = 0; state < num_states; ++state) {
        int g = ts.get_init_distance(state);
        int h = ts.get_goal_distance(state);
        if (g != INF && h != INF) {
            int f = g + h;
            assert(in_bounds(f, states_by_f_and_h));
            assert(in_bounds(h, states_by_f_and_h[f]));
            Bucket &bucket = states_by_f_and_h[f][h];
            if (bucket.empty())
                ++bucket_count;
            bucket.push_back(state);
        }
    }

    buckets.reserve(bucket_count);
    int f_init = (f_start == HIGH ? ts.get_max_f() : 0);
    int f_end = (f_start == HIGH ? 0 : ts.get_max_f());
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
        "This shrink strategy implements the algorithm described in the paper:\n\n"
        " * Malte Helmert, Patrik Haslum and Joerg Hoffmann.<<BR>>\n"
        " [Flexible Abstraction Heuristics for Optimal Sequential Planning "
        "http://ai.cs.unibas.ch/papers/helmert-et-al-icaps2007.pdf].<<BR>>\n "
        "In //Proceedings of the Seventeenth International Conference on "
        "Automated Planning and Scheduling (ICAPS 2007)//, pp. 176-183. 2007. ");
    parser.document_note(
        "shrink_fh(max_states=N)",
        "f-preserving shrinking of transition systems "
        "(called HHH in the IJCAI 2011 paper, see shrink_bisimulation). "
        "Here, N is a numerical parameter for which sensible values "
        "include 1000, 10000, 50000, 100000 and 200000. "
        "Combine this with the linear merge strategy "
        "CG_GOAL_LEVEL to match the heuristic in the paper. "
        "When we last ran experiments on interaction of shrink strategies "
        "with label reduction, this strategy performed best when used with "
        "label reduction before merging (and no label reduction before "
        "shrinking).");
    ShrinkStrategy::add_options_to_parser(parser);
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

    ShrinkStrategy::handle_option_defaults(opts);

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ShrinkFH>(opts);
}

static PluginShared<ShrinkStrategy> _plugin("shrink_fh", _parse);
