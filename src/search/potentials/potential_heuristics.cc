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
PotentialHeuristics::PotentialHeuristics(const Options &options)
    : Heuristic(options),
      optimizer(options),
      optimization_function(OptimizationFunction(options.get_enum("optimization_function"))),
      size(options.get<int>("size")),
      opts(options),
      num_samples(options.get<int>("num_samples")),
      max_potential(options.get<double>("max_potential")),
      max_filtering_time(opts.get<double>("max_filtering_time")),
      max_covering_time(opts.get<double>("max_covering_time")),
      debug(options.get<bool>("debug")) {
}

PotentialHeuristics::~PotentialHeuristics() {
}

void PotentialHeuristics::initialize() {
    Timer initialization_timer;
    verify_no_axioms_no_conditional_effects();
    if (size >= 1) {
        heuristics.reserve(size);
        for (int i = 0; i < size; ++i) {
            optimizer.optimize_for_state(g_initial_state());
            shared_ptr<Heuristic> sampling_heuristic = optimizer.get_heuristic();
            StateRegistry sample_registry;
            vector<GlobalState> samples = sample_states_with_random_walks(
                sample_registry, num_samples, *sampling_heuristic);
            optimizer.optimize_for_samples(samples);
            heuristics.push_back(optimizer.get_heuristic());
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
    for (shared_ptr<Heuristic> heuristic : heuristics) {
        if (eval_context.is_heuristic_infinite(heuristic.get())) {
            return DEAD_END;
        } else {
            value = max(value, eval_context.get_heuristic_value(heuristic.get()));
        }
    }
    return value;
}

void PotentialHeuristics::filter_samples(
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
            sample_to_max_h[sample.get_id()] = eval_context.get_heuristic_value(single_heuristic.get());
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
        const unordered_map<StateID, int> &sample_to_max_h,
        const std::shared_ptr<Heuristic> heuristic,
        unordered_map<StateID, shared_ptr<Heuristic> > &single_heuristics) const {
    vector<GlobalState> suboptimal_samples;
    for (const GlobalState &sample : samples) {
        int max_h = sample_to_max_h.at(sample.get_id());
        int h = EvaluationContext(sample).get_heuristic_value(heuristic.get());
        assert(h <= max_h);
        if (debug)
            cout << h << "/" << max_h << endl;
        if (h == max_h) {
            single_heuristics.erase(sample.get_id());
        } else {
            suboptimal_samples.push_back(sample);
        }
    }
    swap(samples, suboptimal_samples);
}

void PotentialHeuristics::find_complementary_heuristics() {
    // Sample states.
    optimizer.optimize_for_state(g_initial_state());
    shared_ptr<Heuristic> sampling_heuristic = optimizer.get_heuristic();
    StateRegistry sample_registry;
    vector<GlobalState> samples = sample_states_with_random_walks(
        sample_registry, num_samples, *sampling_heuristic);

    // Filter dead ends.
    unordered_map<StateID, int> sample_to_max_h;
    unordered_map<StateID, shared_ptr<Heuristic> > single_heuristics;
    filter_samples(samples, sample_to_max_h, single_heuristics);

    // Cover samples.
    CountdownTimer covering_timer(max_covering_time);
    size_t max_num_heuristics = (size == 0) ? numeric_limits<size_t>::max() : abs(size);
    while (!samples.empty() && heuristics.size() < max_num_heuristics) {
        if (covering_timer.is_expired()) {
            cout << "Ran out of time covering samples." << endl;
        }
        cout << "Create heuristic #" << heuristics.size() + 1 << endl;
        optimizer.optimize_for_samples(samples);
        shared_ptr<Heuristic> group_heuristic = optimizer.get_heuristic();
        size_t last_num_samples = samples.size();
        cover_samples(samples, sample_to_max_h, group_heuristic, single_heuristics);
        if (samples.size() < last_num_samples) {
            heuristics.push_back(group_heuristic);
        } else {
            cout << "No sample was removed -> Choose a precomputed heuristic." << endl;
            StateID state_id = g_rng.choose(samples)->get_id();
            shared_ptr<Heuristic> single_heuristic = single_heuristics[state_id];
            single_heuristics.erase(state_id);
            cover_samples(samples, sample_to_max_h, single_heuristic, single_heuristics);
            heuristics.push_back(single_heuristic);
        }
        cout << "Removed " << last_num_samples - samples.size()
             << " samples. " << samples.size() << " remaining." << endl;
    }
    if (samples.empty()) {
        assert(single_heuristics.empty());
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
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return new PotentialHeuristics(opts);
}

static Plugin<Heuristic> _plugin("potentials", _parse);
}
