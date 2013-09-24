#include "shrink_fh.h"

#include "abstraction.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>
#include <map>
#include <vector>

using namespace std;


static const int infinity = numeric_limits<int>::max();


ShrinkFH::ShrinkFH(const Options &opts)
    : ShrinkBucketBased(opts),
      f_start(HighLow(opts.get_enum("shrink_f"))),
      h_start(HighLow(opts.get_enum("shrink_h"))) {
}

ShrinkFH::~ShrinkFH() {
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

void ShrinkFH::partition_into_buckets(
    const Abstraction &abs, vector<Bucket> &buckets) const {
    assert(buckets.empty());
    // The following line converts to double to avoid overflow.
    int max_f = abs.get_max_f();
    if (static_cast<double>(max_f) * max_f / 2.0 > abs.size()) {
        // Use map because an average bucket in the vector structure
        // would contain less than 1 element (roughly).
        ordered_buckets_use_map(abs, buckets);
    } else {
        ordered_buckets_use_vector(abs, buckets);
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
    const Abstraction &abs,
    vector<Bucket> &buckets) const {
    map<int, map<int, Bucket > > states_by_f_and_h;
    int bucket_count = 0;
    for (AbstractStateRef state = 0; state < abs.size(); ++state) {
        int g = abs.get_init_distance(state);
        int h = abs.get_goal_distance(state);
        if (g != infinity && h != infinity) {
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
    assert(buckets.size() == bucket_count);
}

void ShrinkFH::ordered_buckets_use_vector(
    const Abstraction &abs,
    vector<Bucket> &buckets) const {
    vector<vector<Bucket > > states_by_f_and_h;
    states_by_f_and_h.resize(abs.get_max_f() + 1);
    for (int f = 0; f <= abs.get_max_f(); ++f)
        states_by_f_and_h[f].resize(min(f, abs.get_max_h()) + 1);
    int bucket_count = 0;
    for (AbstractStateRef state = 0; state < abs.size(); ++state) {
        int g = abs.get_init_distance(state);
        int h = abs.get_goal_distance(state);
        if (g != infinity && h != infinity) {
            int f = g + h;
            assert(f >= 0 && f < states_by_f_and_h.size());
            assert(h >= 0 && h < states_by_f_and_h[f].size());
            Bucket &bucket = states_by_f_and_h[f][h];
            if (bucket.empty())
                ++bucket_count;
            bucket.push_back(state);
        }
    }

    buckets.reserve(bucket_count);
    int f_init = (f_start == HIGH ? abs.get_max_f() : 0);
    int f_end = (f_start == HIGH ? 0 : abs.get_max_f());
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
    assert(buckets.size() == bucket_count);
}

ShrinkStrategy *ShrinkFH::create_default(int max_states) {
    Options opts;
    opts.set("max_states", max_states);
    opts.set("max_states_before_merge", max_states);
    opts.set<int>("shrink_f", ShrinkFH::HIGH);
    opts.set<int>("shrink_h", ShrinkFH::LOW);
    return new ShrinkFH(opts);
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    parser.document_synopsis("f-Preserving", "");
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
        return 0;

    ShrinkStrategy::handle_option_defaults(opts);

    if (!parser.dry_run())
        return new ShrinkFH(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_fh", _parse);
