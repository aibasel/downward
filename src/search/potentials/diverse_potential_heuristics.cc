#include "diverse_potential_heuristics.h"

#include "potential_function.h"
#include "potential_max_heuristic.h"
#include "util.h"

#include "../countdown_timer.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"

#include <unordered_set>

using namespace std;


namespace potentials {
DiversePotentialHeuristics::DiversePotentialHeuristics(const Options &opts)
    : optimizer(opts),
      max_num_heuristics(opts.get<int>("max_num_heuristics")),
      num_samples(opts.get<int>("num_samples")),
      max_filtering_time(opts.get<double>("max_filtering_time")),
      max_covering_time(opts.get<double>("max_covering_time")) {
    Timer init_timer;
    find_diverse_functions();
    cout << "Potential heuristics: " << diverse_functions.size() << endl;
    cout << "Initialization of potential heuristics: " << init_timer << endl;
}

SamplesAndFunctions DiversePotentialHeuristics::filter_samples_and_compute_functions(
    const vector<State> &samples) {
    CountdownTimer filtering_timer(max_filtering_time);
    unordered_set<State> dead_ends;
    int num_duplicates = 0;
    int num_dead_ends = 0;
    SamplesAndFunctions samples_and_functions;
    for (const State &sample : samples) {
        if (filtering_timer.is_expired()) {
            cout << "Ran out of time filtering dead ends." << endl;
            break;
        }
        // Skipping duplicates is not necessary, but saves LP evaluations.
        if (samples_and_functions.count(sample) || dead_ends.count(sample)) {
            ++num_duplicates;
            continue;
        }
        optimizer.optimize_for_state(sample);
        if (optimizer.has_optimal_solution()) {
            samples_and_functions[sample] = optimizer.get_potential_function();
        } else {
            dead_ends.insert(sample);
            ++num_dead_ends;
        }
    }
    cout << "Time for filtering dead ends: " << filtering_timer << endl;
    cout << "Duplicate samples: " << num_duplicates << endl;
    cout << "Dead end samples: " << num_dead_ends << endl;
    cout << "Unique non-dead-end samples: " << samples_and_functions.size() << endl;
    assert(num_duplicates + num_dead_ends + samples_and_functions.size() == samples.size());
    return samples_and_functions;
}

void DiversePotentialHeuristics::filter_covered_samples(
    const PotentialFunction &chosen_function,
    SamplesAndFunctions &samples_and_functions) const {
    for (auto it = samples_and_functions.begin();
         it != samples_and_functions.end();) {
        const State &sample = it->first;
        Function sample_function = it->second;
        int max_h = sample_function->get_value(sample);
        int h = chosen_function.get_value(sample);
        assert(h <= max_h);
        // TODO: Count as covered if max_h <= 0.
        if (h == max_h) {
            it = samples_and_functions.erase(it);
        }
    }
}

Function DiversePotentialHeuristics::find_function_and_remove_covered_samples(
    SamplesAndFunctions &samples_and_functions) {
    vector<State> samples;
    for (auto &sample_and_function : samples_and_functions) {
        const State &state = sample_and_function.first;
        samples.push_back(state);
    }
    optimizer.optimize_for_samples(samples);
    Function function = optimizer.get_potential_function();
    size_t last_num_samples = samples_and_functions.size();
    filter_covered_samples(*function, samples_and_functions);
    if (samples_and_functions.size() == last_num_samples) {
        cout << "No sample removed -> Use arbitrary precomputed function."
             << endl;
        function = samples_and_functions.begin()->second;
        filter_covered_samples(*function, samples_and_functions);
    }
    cout << "Removed " << last_num_samples - samples_and_functions.size()
         << " samples. " << samples_and_functions.size() << " remaining."
         << endl;
    return function;
}

void DiversePotentialHeuristics::cover_samples(
    SamplesAndFunctions &samples_and_functions) {
    CountdownTimer covering_timer(max_covering_time);
    while (!samples_and_functions.empty() &&
           static_cast<int>(diverse_functions.size()) < max_num_heuristics) {
        if (covering_timer.is_expired()) {
            cout << "Ran out of time covering samples." << endl;
            break;
        }
        cout << "Find heuristic #" << diverse_functions.size() + 1 << endl;
        diverse_functions.push_back(
            find_function_and_remove_covered_samples(samples_and_functions));
    }
    cout << "Time for covering samples: " << covering_timer << endl;
}

void DiversePotentialHeuristics::find_diverse_functions() {
    // Sample states.
    vector<State> samples = sample_without_dead_end_detection(
        optimizer, num_samples);

    // Filter dead end samples.
    SamplesAndFunctions samples_and_functions =
        filter_samples_and_compute_functions(samples);

    // Iteratively cover samples.
    cover_samples(samples_and_functions);
}

vector<Function > DiversePotentialHeuristics::get_functions() const {
    return diverse_functions;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Diverse potential heuristics", "");
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample",
        "1000",
        Bounds("0", "infinity"));
    parser.add_option<int>(
        "max_num_heuristics",
        "maximum number of potential heuristics",
        "infinity",
        Bounds("0", "infinity"));
    parser.add_option<double>(
        "max_filtering_time",
        "time limit in seconds for filtering dead end samples",
        "infinity",
        Bounds("0.0", "infinity"));
    parser.add_option<double>(
        "max_covering_time",
        "time limit in seconds for covering samples",
        "infinity",
        Bounds("0.0", "infinity"));
    add_common_potentials_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    DiversePotentialHeuristics factory(opts);
    opts.set<vector<Function> >("functions", factory.get_functions());
    return new PotentialMaxHeuristic(opts);
}

static Plugin<Heuristic> _plugin("diverse_potentials", _parse);
}
