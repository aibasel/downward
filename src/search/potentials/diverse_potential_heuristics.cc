#include "diverse_potential_heuristics.h"

#include "potential_function.h"
#include "potential_heuristics.h"

#include "../countdown_timer.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../sampling.h"
#include "../state_registry.h"
#include "../timer.h"

using namespace std;


namespace potentials {
DiversePotentialHeuristics::DiversePotentialHeuristics(const Options &opts)
    : optimizer(opts),
      max_num_heuristics(opts.get<int>("max_num_heuristics")),
      num_samples(opts.get<int>("num_samples")),
      max_filtering_time(opts.get<double>("max_filtering_time")),
      max_covering_time(opts.get<double>("max_covering_time")) {
    Timer initialization_timer;
    find_diverse_heuristics();
    cout << "Potential heuristics: " << diverse_functions.size() << endl;
    cout << "Initialization of potential heuristics: " << initialization_timer << endl;
}

void DiversePotentialHeuristics::filter_dead_ends_and_duplicates(
    vector<GlobalState> &samples) {
    assert(sample_to_max_h.empty());
    assert(single_functions.empty());
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
        if (single_functions.count(sample.get_id()) != 0) {
            ++num_duplicates;
            continue;
        }
        bool optimal = optimizer.optimize_for_state(sample);
        if (optimal) {
            dead_end_free_samples.push_back(sample);
            shared_ptr<PotentialFunction> function = optimizer.get_potential_function();
            sample_to_max_h[sample.get_id()] = function->get_value(sample);
            single_functions[sample.get_id()] = function;
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

void DiversePotentialHeuristics::filter_covered_samples(
    const shared_ptr<PotentialFunction> function,
    vector<GlobalState> &samples) {
    vector<GlobalState> not_covered_samples;
    for (const GlobalState &sample : samples) {
        int max_h = sample_to_max_h.at(sample.get_id());
        int h = function->get_value(sample);
        assert(h <= max_h);
        // TODO: Count as covered if max_h <= 0.
        if (h == max_h) {
            sample_to_max_h.erase(sample.get_id());
            single_functions.erase(sample.get_id());
        } else {
            not_covered_samples.push_back(sample);
        }
    }
    swap(samples, not_covered_samples);
}

shared_ptr<PotentialFunction> DiversePotentialHeuristics::find_function_and_remove_covered_samples(
    vector<GlobalState> &samples) {
    optimizer.optimize_for_samples(samples);
    shared_ptr<PotentialFunction> group_function = optimizer.get_potential_function();
    size_t last_num_samples = samples.size();
    filter_covered_samples(group_function, samples);
    shared_ptr<PotentialFunction> function = nullptr;
    if (samples.size() < last_num_samples) {
        function = group_function;
    } else {
        cout << "No sample was removed -> Use a precomputed function." << endl;
        StateID state_id = g_rng.choose(samples)->get_id();
        shared_ptr<PotentialFunction> single_heuristic = single_functions[state_id];
        single_functions.erase(state_id);
        filter_covered_samples(single_heuristic, samples);
        function = single_heuristic;
    }
    cout << "Removed " << last_num_samples - samples.size() << " samples. "
         << samples.size() << " remaining." << endl;
    return function;
}

void DiversePotentialHeuristics::cover_samples(vector<GlobalState> &samples) {
    CountdownTimer covering_timer(max_covering_time);
    while (!samples.empty() &&
           static_cast<int>(diverse_functions.size()) < max_num_heuristics) {
        if (covering_timer.is_expired()) {
            cout << "Ran out of time covering samples." << endl;
            break;
        }
        cout << "Find heuristic #" << diverse_functions.size() + 1 << endl;
        diverse_functions.push_back(
            find_function_and_remove_covered_samples(samples));
    }
    cout << "Time for covering samples: " << covering_timer << endl;
}

void DiversePotentialHeuristics::find_diverse_heuristics() {
    // Sample states.
    optimizer.optimize_for_state(g_initial_state());
    shared_ptr<Heuristic> sampling_heuristic = optimizer.get_heuristic();
    StateRegistry sample_registry;
    vector<GlobalState> samples = sample_states_with_random_walks(
        sample_registry, num_samples, *sampling_heuristic);

    // Filter dead end samples.
    filter_dead_ends_and_duplicates(samples);

    // Iteratively cover samples.
    cover_samples(samples);
}

vector<shared_ptr<PotentialFunction> > DiversePotentialHeuristics::get_functions() const {
    return diverse_functions;
}

static Heuristic *_parse(OptionParser &parser) {
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
    add_common_potentials_options_to_parser(parser);
    add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    DiversePotentialHeuristics factory(opts);

    Options options;
    options.set<int>("cost_type", NORMAL);
    options.set<vector<shared_ptr<PotentialFunction> > >(
        "functions", factory.get_functions());
    return new PotentialHeuristics(options);
}

static Plugin<Heuristic> _plugin("diverse_potentials", _parse);
}
