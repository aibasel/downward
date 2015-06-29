#include "potential_heuristics.h"

#include "potential_optimizer.h"

#include "../countdown_timer.h"
#include "../globals.h"
#include "../plugin.h"
#include "../rng.h"
#include "../sampling.h"
#include "../state_registry.h"
#include "../timer.h"

#include <cmath>
#include <numeric>
#include <unordered_map>

using namespace std;


namespace potentials {
PotentialHeuristics::PotentialHeuristics(const Options &opts)
    : Heuristic(opts),
      diversify(opts.get<bool>("diversify")),
      max_num_heuristics(opts.get<int>("max_num_heuristics")),
      num_samples(opts.get<int>("num_samples")),
      max_filtering_time(opts.get<double>("max_filtering_time")),
      max_covering_time(opts.get<double>("max_covering_time")),
      debug(opts.get<bool>("debug")),
      optimizer(opts) {
}

void PotentialHeuristics::initialize() {
    Timer initialization_timer;
    verify_no_axioms_no_conditional_effects();
    if (diversify) {
        find_diverse_heuristics();
    } else {
        for (int i = 0; i < max_num_heuristics; ++i) {
            optimize_for_samples(optimizer, num_samples);
            heuristics.push_back(optimizer.get_heuristic());
        }
    }
    cout << "Potential heuristics: " << heuristics.size() << endl;
    cout << "Initialization of potential heuristics: " << initialization_timer << endl;
}

int PotentialHeuristics::compute_heuristic(const GlobalState &state) {
    EvaluationContext eval_context(state);
    int value = 0;
    for (shared_ptr<Heuristic> heuristic : heuristics) {
        if (eval_context.is_heuristic_infinite(heuristic.get())) {
            return DEAD_END;
        } else {
            value = max(value, eval_context.get_heuristic_value(heuristic.get()));
        }
    }
    return value;
}

void PotentialHeuristics::filter_dead_ends_and_duplicates(
        vector<GlobalState> &samples,
        unordered_map<StateID, int> &sample_to_max_h,
        unordered_map<StateID, shared_ptr<Heuristic> > &single_heuristics) {
    assert(sample_to_max_h.empty());
    assert(single_heuristics.empty());
    CountdownTimer filtering_timer(max_filtering_time);
    int num_duplicates = 0;
    int num_dead_ends = 0;
    vector<GlobalState> dead_end_free_samples;
    for (const GlobalState &sample : samples) {
        if (filtering_timer.is_expired()) {
            cout << "Ran out of time filtering dead ends." << endl;
            break;
        }
        // Skipping duplicates is not necessary, but saves LP evaluations.
        if (single_heuristics.count(sample.get_id()) != 0) {
            ++num_duplicates;
            continue;
        }
        bool optimal = optimizer.optimize_for_state(sample);
        if (optimal) {
            dead_end_free_samples.push_back(sample);
            shared_ptr<Heuristic> single_heuristic = optimizer.get_heuristic();
            EvaluationContext eval_context(sample);
            sample_to_max_h[sample.get_id()] = eval_context.get_heuristic_value(
                single_heuristic.get());
            single_heuristics[sample.get_id()] = single_heuristic;
        } else {
            ++num_dead_ends;
        }
    }
    cout << "Time for filtering dead ends: " << filtering_timer << endl;
    cout << "Duplicate samples: " << num_duplicates << endl;
    cout << "Dead end samples: " << num_dead_ends << endl;
    cout << "Unique non-dead-end samples: " << dead_end_free_samples.size()
         << endl;
    assert(num_duplicates + num_dead_ends + dead_end_free_samples.size() ==
        samples.size());
    swap(samples, dead_end_free_samples);
}

void PotentialHeuristics::filter_covered_samples(
        const std::shared_ptr<Heuristic> heuristic,
        vector<GlobalState> &samples,
        unordered_map<StateID, int> &sample_to_max_h,
        unordered_map<StateID, shared_ptr<Heuristic> > &single_heuristics) const {
    vector<GlobalState> not_covered_samples;
    for (const GlobalState &sample : samples) {
        int max_h = sample_to_max_h.at(sample.get_id());
        int h = EvaluationContext(sample).get_heuristic_value(heuristic.get());
        assert(h <= max_h);
        if (debug)
            cout << h << "/" << max_h << endl;
        if (h == max_h) {
            sample_to_max_h.erase(sample.get_id());
            single_heuristics.erase(sample.get_id());
        } else {
            not_covered_samples.push_back(sample);
        }
    }
    swap(samples, not_covered_samples);
}

void PotentialHeuristics::cover_samples(
        vector<GlobalState> &samples,
        unordered_map<StateID, int> &sample_to_max_h,
        unordered_map<StateID, shared_ptr<Heuristic> > &single_heuristics) {
    CountdownTimer covering_timer(max_covering_time);
    while (!samples.empty() && static_cast<int>(heuristics.size()) < max_num_heuristics) {
        if (covering_timer.is_expired()) {
            cout << "Ran out of time covering samples." << endl;
            break;
        }
        cout << "Create heuristic #" << heuristics.size() + 1 << endl;
        optimizer.optimize_for_samples(samples);
        shared_ptr<Heuristic> group_heuristic = optimizer.get_heuristic();
        size_t last_num_samples = samples.size();
        filter_covered_samples(group_heuristic, samples, sample_to_max_h, single_heuristics);
        if (samples.size() < last_num_samples) {
            heuristics.push_back(group_heuristic);
        } else {
            cout << "No sample was removed -> Choose a precomputed heuristic." << endl;
            StateID state_id = g_rng.choose(samples)->get_id();
            shared_ptr<Heuristic> single_heuristic = single_heuristics[state_id];
            single_heuristics.erase(state_id);
            filter_covered_samples(single_heuristic, samples, sample_to_max_h, single_heuristics);
            heuristics.push_back(single_heuristic);
        }
        cout << "Removed " << last_num_samples - samples.size()
             << " samples. " << samples.size() << " remaining." << endl;
    }
    cout << "Time for covering samples: " << covering_timer << endl;
}

void PotentialHeuristics::find_diverse_heuristics() {
    // Sample states.
    optimizer.optimize_for_state(g_initial_state());
    shared_ptr<Heuristic> sampling_heuristic = optimizer.get_heuristic();
    StateRegistry sample_registry;
    vector<GlobalState> samples = sample_states_with_random_walks(
        sample_registry, num_samples, *sampling_heuristic);

    // Filter dead end samples.
    unordered_map<StateID, int> sample_to_max_h;
    unordered_map<StateID, shared_ptr<Heuristic> > single_heuristics;
    filter_dead_ends_and_duplicates(samples, sample_to_max_h, single_heuristics);

    // Iteratively cover samples.
    cover_samples(samples, sample_to_max_h, single_heuristics);
}

static Heuristic *_parse(OptionParser &parser) {
    add_lp_solver_option_to_parser(parser);
    add_common_potential_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    parser.add_option<bool>(
        "diversify",
        "use automatic diversification",
        "true");
    parser.add_option<int>(
        "max_num_heuristics",
        "maximum number of potential heuristics",
        "infinity");
    parser.add_option<double>(
        "max_filtering_time",
        "time limit in seconds for filtering dead end samples",
        "infinity");
    parser.add_option<double>(
        "max_covering_time",
        "time limit in seconds for covering samples",
        "infinity");
    Options opts = parser.parse();
    if (!opts.get<bool>("diversify") &&
        opts.get<int>("max_num_heuristics") == numeric_limits<int>::max()) {
        parser.error("Either use diversify=true or specify max_num_heuristics");
    }
    if (parser.dry_run())
        return nullptr;
    else
        return new PotentialHeuristics(opts);
}

static Plugin<Heuristic> _plugin("diverse_potentials", _parse);
}
