#include "potential_heuristics.h"

#include "../countdown_timer.h"
#include "../globals.h"
#include "../plugin.h"
#include "../rng.h"
#include "../timer.h"

#include <cmath>
#include <numeric>
#include <unordered_map>

using namespace std;


namespace potentials {
PotentialHeuristics::PotentialHeuristics(const Options &options)
    : Heuristic(options),
      optimization_function(OptimizationFunction(options.get_enum("optimization_function"))),
      cover_strategy(CoverStrategy(options.get_enum("cover_strategy"))),
      size(options.get<int>("size")),
      opts(options),
      num_samples(options.get<int>("num_samples")),
      max_potential(options.get<double>("max_potential")),
      max_filtering_time(opts.get<double>("max_filtering_time")),
      max_covering_time(opts.get<double>("max_covering_time")),
      debug(options.get<bool>("debug")) {
}

PotentialHeuristics::~PotentialHeuristics() {
    for (size_t i = 0; i < heuristics.size(); ++i) {
        delete heuristics[i];
    }
}

void PotentialHeuristics::initialize() {
    Timer initialization_timer;
    verify_no_axioms_no_conditional_effects();
    Options options(opts);
    if (size >= 1) {
        heuristics.reserve(size);
        for (int i = 0; i < size; ++i) {
            PotentialHeuristic *heuristic = new PotentialHeuristic(options);

            // HACK: Initialize heuristic.
            EvaluationContext eval_context(g_initial_state());
            eval_context.is_heuristic_infinite(heuristic);

            heuristic->release_memory();
            heuristics.push_back(heuristic);
        }
    } else {
        find_complementary_heuristics();
    }
    cout << "Potential heuristics: " << heuristics.size() << endl;
    cout << "Initialization of PotentialHeuristics: " << initialization_timer << endl;
}

int PotentialHeuristics::compute_heuristic(const GlobalState &state) {
    EvaluationContext eval_context(state);
    int value = 0;
    for (size_t i = 0; i < heuristics.size(); ++i) {
        PotentialHeuristic *heuristic = heuristics[i];
        assert(heuristic);

        if (eval_context.is_heuristic_infinite(heuristic)) {
            return DEAD_END;
        } else {
            value = max(value, eval_context.get_heuristic_value(heuristic));
        }
    }
    return value;
}

void PotentialHeuristics::filter_samples(
        vector<GlobalState> &samples,
        unordered_map<StateID, int, hash_state_id> &sample_to_max_h,
        PotentialHeuristic &heuristic,
        unordered_map<StateID, PotentialHeuristic *, hash_state_id> &single_heuristics) const {
    assert(sample_to_max_h.empty());
    assert(single_heuristics.empty());
    CountdownTimer filtering_timer(max_filtering_time);
    int num_duplicates = 0;
    int num_dead_ends = 0;
    vector<GlobalState> dead_end_free_samples;
    for (size_t i = 0; i < samples.size(); ++i) {
        if (filtering_timer.is_expired()) {
            cout << "Ran out of time filtering dead ends." << endl;
            break;
        }
        const GlobalState &sample = samples[i];
        if (single_heuristics.find(sample.get_id()) != single_heuristics.end()) {
            ++num_duplicates;
            continue;
        }
        bool feasible = heuristic.optimize_potential_for_state(sample);
        if (feasible) {
            dead_end_free_samples.push_back(sample);
            PotentialHeuristic *single_heuristic = new PotentialHeuristic(heuristic);
            single_heuristic->lp_solver = 0;
            sample_to_max_h[sample.get_id()] = single_heuristic->compute_heuristic(sample);
            single_heuristics.insert(make_pair(sample.get_id(), single_heuristic));
        } else {
            ++num_dead_ends;
        }
    }
    cout << "Time for filtering dead ends: " << filtering_timer << endl;
    cout << "Duplicate samples: " << num_duplicates << endl;
    cout << "Dead end samples: " << num_dead_ends << endl;
    cout << "Unique non-dead-end samples: " << dead_end_free_samples.size() << endl;
    swap(samples, dead_end_free_samples);
}

void PotentialHeuristics::cover_samples(
        vector<GlobalState> &samples,
        const unordered_map<StateID, int, hash_state_id> &sample_to_max_h,
        PotentialHeuristic &heuristic,
        unordered_map<StateID, PotentialHeuristic *, hash_state_id> &single_heuristics) const {
    vector<GlobalState> suboptimal_samples;
    for (size_t i = 0; i < samples.size(); ++i) {
        int max_h = sample_to_max_h.at(samples[i].get_id());
        int h = heuristic.compute_heuristic(samples[i]);
        assert(h <= max_h);
        if (debug)
            cout << h << "/" << max_h << endl;
        if (h == max_h) {
            delete single_heuristics[samples[i].get_id()];
            single_heuristics.erase(samples[i].get_id());
        } else {
            suboptimal_samples.push_back(samples[i]);
        }
    }
    swap(samples, suboptimal_samples);
}

void PotentialHeuristics::find_complementary_heuristics() {
    Options options(opts);
    options.set<int>("optimization_function", INITIAL_STATE);
    PotentialHeuristic helper_heuristic(options);

    // HACK: Initialize heuristic.
    EvaluationContext eval_context(g_initial_state());
    eval_context.is_heuristic_infinite(&helper_heuristic);

    StateRegistry sample_registry;
    vector<GlobalState> samples;
    unordered_map<StateID, int, hash_state_id> sample_to_max_h;
    unordered_map<StateID, PotentialHeuristic *, hash_state_id> single_heuristics;

    Heuristic *heuristic = opts.get<Heuristic *>("sampling_heuristic");
    helper_heuristic.sample_states(sample_registry, samples, num_samples, heuristic);
    filter_samples(samples, sample_to_max_h, helper_heuristic, single_heuristics);

    CountdownTimer covering_timer(max_covering_time);
    size_t max_num_heuristics = (size == 0) ? numeric_limits<size_t>::max() : abs(size);
    size_t last_num_samples = samples.size();
    while (!samples.empty() && heuristics.size() < max_num_heuristics) {
        if (covering_timer.is_expired()) {
            cout << "Ran out of time covering samples." << endl;
            for (auto it = single_heuristics.begin(); it != single_heuristics.end(); ++it) {
                delete it->second;
            }
            single_heuristics.clear();
            break;
        }
        cout << "Create heuristic #" << heuristics.size() + 1 << endl;
        helper_heuristic.optimize_potential_for_samples(samples);
        cover_samples(samples, sample_to_max_h, helper_heuristic,
                      single_heuristics);
        if (samples.size() == last_num_samples) {
            cout << "No sample was removed -> Choose a precomputed heuristic." << endl;
            StateID state_id = StateID::no_state;
            if (cover_strategy == RANDOM) {
                state_id = g_rng.choose(samples)->get_id();
            }
            assert(state_id != StateID::no_state);
            PotentialHeuristic *single_heuristic = single_heuristics[state_id];
            heuristics.push_back(single_heuristic);
            single_heuristics.erase(state_id);
            cover_samples(samples, sample_to_max_h, *single_heuristic, single_heuristics);
        } else {
            PotentialHeuristic *group_heuristic = new PotentialHeuristic(helper_heuristic);
            heuristics.push_back(group_heuristic);
        }
        if (debug)
            heuristics.back()->dump_potentials();
        cout << "Removed " << last_num_samples - samples.size()
             << " samples. " << samples.size() << " remaining." << endl;

        last_num_samples = samples.size();
    }
    if (samples.empty()) {
        assert(single_heuristics.empty());
    } else {
        for (auto it : single_heuristics)
            delete it.second;
        single_heuristics.clear();
    }
    cout << "Time for covering samples: " << covering_timer << endl;
}

static Heuristic *_parse(OptionParser &parser) {
    add_lp_solver_option_to_parser(parser);
    add_common_potential_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    parser.add_option<int>(
        "size",
        "Number of potential heuristics to create. Use 0 to determine size automatically.",
        "1");
    parser.add_option<double>(
        "max_covering_time",
        "time limit in seconds for covering samples",
        "infinity");
    vector<string> cover_strategy;
    vector<string> cover_strategy_doc;
    cover_strategy.push_back("RANDOM");
    cover_strategy_doc.push_back(
        "add function for random sample");
    parser.add_enum_option("cover_strategy",
                           cover_strategy,
                           "Which potential function for a single sample to choose.",
                           "RANDOM",
                           cover_strategy_doc);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    if (!opts.contains("sampling_heuristic")) {
        opts.set<Heuristic *>("sampling_heuristic", 0);
    }
    return new PotentialHeuristics(opts);
}

static Plugin<Heuristic> _plugin("potentials", _parse);
}
