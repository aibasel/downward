#include "merge_and_shrink_heuristic.h"

#include "labels.h"
#include "merge_strategy.h"
#include "shrink_strategy.h"
#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"
#include "../timer.h"
#include "../utilities.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      merge_strategy(opts.get<MergeStrategy *>("merge_strategy")),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      labels(opts.get<Labels *>("label_reduction")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
    merge_strategy->initialize(task);
    labels->initialize(task_proxy);
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
    delete merge_strategy;
    delete shrink_strategy;
    delete labels;
}

void MergeAndShrinkHeuristic::report_peak_memory_delta(bool final) const {
    if (final)
        cout << "Final";
    else
        cout << "Current";
    cout << " peak memory of merge-and-shrink computation: "
         << get_peak_memory_in_kb() - starting_peak_memory << " KB" << endl;
}

void MergeAndShrinkHeuristic::dump_options() const {
    merge_strategy->dump_options();
    shrink_strategy->dump_options();
    labels->dump_label_reduction_options();
    cout << "Expensive statistics: "
         << (use_expensive_statistics ? "enabled" : "disabled") << endl;
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    string dashes(79, '=');
    if (use_expensive_statistics) {
        cerr << dashes << endl
             << "WARNING! You have enabled extra statistics for "
        "merge-and-shrink heuristics.\n"
        "These statistics require a lot of time and memory.\n"
        "When last tested (around revision 3011), enabling the "
        "extra statistics\nincreased heuristic generation time by "
        "76%. This figure may be significantly\nworse with more "
        "recent code or for particular domains and instances.\n"
        "You have been warned. Don't use this for benchmarking!"
        << endl << dashes << endl;
    }
    if (!labels->reduce_before_merging() && !labels->reduce_before_shrinking()) {
        cerr << dashes << endl
             << "WARNING! You did not enable label reduction. This may\n"
        "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    } else if (labels->reduce_before_merging() && labels->reduce_before_shrinking()) {
        cerr << dashes << endl
             << "WARNING! You set label reduction to be applied twice in\n"
        "each merge-and-shrink iteration, both before shrinking and\n"
        "merging. This double computation effort does not pay off\n"
        "for most configurations !"
        << endl << dashes << endl;
    } else {
        if (labels->reduce_before_shrinking() &&
            (shrink_strategy->get_name() == "f-preserving"
             || shrink_strategy->get_name() == "random")) {
            cerr << dashes << endl
                 << "WARNING! Bucket-based shrink strategies such as\n"
            "f-preserving random perform best if used with label\n"
            "reduction before merging, not before shrinking!"
            << endl << dashes << endl;
        }
        if (labels->reduce_before_merging() &&
            shrink_strategy->get_name() == "bisimulation") {
            cerr << dashes << endl
                 << "WARNING! Shrinking based on bisimulation performs best\n"
            "if used with label reduction before shrinking, not\n"
            "before merging!"
            << endl << dashes << endl;
        }
    }
}

TransitionSystem *MergeAndShrinkHeuristic::build_transition_system(const Timer &timer) {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_transition_systems also
    //       allocates memory.

    // Set of all transition systems. Entries with 0 have been merged.
    vector<TransitionSystem *> all_transition_systems;
    size_t num_vars = task_proxy.get_variables().size();
    if (num_vars * 2 - 1 > all_transition_systems.max_size())
        exit_with(EXIT_OUT_OF_MEMORY);
    all_transition_systems.reserve(num_vars * 2 - 1);
    cout << endl;
    TransitionSystem::build_atomic_transition_systems(task_proxy,
                                                      all_transition_systems,
                                                      labels);
    cout << endl;

    cout << "Starting merge-and-shrink main loop..." << endl;
    vector<int> transition_system_order;
    while (!merge_strategy->done()) {
        // Choose next transition systems to merge
        pair<int, int> next_transition_system = merge_strategy->get_next(all_transition_systems);
        int system_one = next_transition_system.first;
        transition_system_order.push_back(system_one);
        TransitionSystem *transition_system = all_transition_systems[system_one];
        assert(transition_system);
        int system_two = next_transition_system.second;
        assert(system_one != system_two);
        transition_system_order.push_back(system_two);
        TransitionSystem *other_transition_system = all_transition_systems[system_two];
        assert(other_transition_system);
        transition_system->statistics(timer, use_expensive_statistics);
        other_transition_system->statistics(timer, use_expensive_statistics);

        if (labels->reduce_before_shrinking()) {
            // Label reduction before shrinking
            labels->reduce(make_pair(system_one, system_two), all_transition_systems);
        }

        /*
          NOTE: both the shrinking strategy classes and the construction of
          the composite require input transition systems to be solvable.
        */
        if (!transition_system->is_solvable())
            return transition_system;
        if (!other_transition_system->is_solvable())
            return other_transition_system;

        // Shrinking
        pair<bool, bool> shrunk = shrink_strategy->shrink_before_merge(*transition_system,
                                                                       *other_transition_system);
        if (shrunk.first)
            transition_system->statistics(timer, use_expensive_statistics);
        if (shrunk.second)
            other_transition_system->statistics(timer, use_expensive_statistics);

        if (labels->reduce_before_merging()) {
            // Label reduction before merging
            labels->reduce(make_pair(system_one, system_two), all_transition_systems);
        }

        // Merging
        TransitionSystem *new_transition_system = new CompositeTransitionSystem(
            task_proxy, labels, transition_system, other_transition_system);
        new_transition_system->statistics(timer, use_expensive_statistics);
        all_transition_systems.push_back(new_transition_system);

        transition_system->release_memory();
        other_transition_system->release_memory();
        all_transition_systems[system_one] = 0;
        all_transition_systems[system_two] = 0;

        report_peak_memory_delta();
        cout << endl;
    }

    assert(all_transition_systems.size() == num_vars * 2 - 1);
    TransitionSystem *final_transition_system = 0;
    for (size_t i = 0; i < all_transition_systems.size(); ++i) {
        if (all_transition_systems[i]) {
            if (final_transition_system) {
                cerr << "Found more than one remaining transition system!" << endl;
                exit_with(EXIT_CRITICAL_ERROR);
            }
            final_transition_system = all_transition_systems[i];
            assert(i == all_transition_systems.size() - 1);
        }
    }

    if (!final_transition_system->is_solvable())
        return final_transition_system;

    final_transition_system->release_memory();

    delete labels;
    labels = 0;

    cout << "Done with merge-and-shrink main loop." << endl;
    cout << "Order of merged transition systems: ";
    for (size_t i = 1; i < transition_system_order.size(); i += 2) {
        cout << transition_system_order[i - 1] << " " << transition_system_order[i] << ", ";
    }
    cout << endl;
    return final_transition_system;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    starting_peak_memory = get_peak_memory_in_kb();
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms(task_proxy);

    final_transition_system = build_transition_system(timer);
    if (!final_transition_system->is_solvable()) {
        cout << "Abstract problem is unsolvable!" << endl;
    }
    cout << "Final transition system size: " << final_transition_system->get_size() << endl;

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl;
    cout << "initial h value: "
    // TODO: after adopting the task interface everywhere, change this
    // back to compute_heuristic(task_proxy.get_initial_state())
    << final_transition_system->get_cost(task_proxy.get_initial_state())
    << endl;
    report_peak_memory_delta(true);
    cout << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    int cost = final_transition_system->get_cost(state);
    if (cost == -1)
        return DEAD_END;
    return cost;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Merge-and-shrink heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
        "Note",
        "Conditional effects are supported directly. Note, however, that "
        "for tasks that are not factored (in the sense of the JACM 2014 "
        "merge-and-shrink paper), the atomic transition systems on which "
        "merge-and-shrink heuristics are based are nondeterministic, "
        "which can lead to poor heuristics even when no shrinking is "
        "performed.");

    // Merge strategy option.
    parser.add_option<MergeStrategy *>(
        "merge_strategy",
        "merge strategy; choose between merge_linear with various variable "
        "orderings and merge_dfp.");

    // Shrink strategy option.
    parser.add_option<ShrinkStrategy *>(
        "shrink_strategy",
        "shrink strategy; choose between shrink_fh and shrink_bisimulation. "
        "A good configuration for bisimulation based shrinking is: "
        "shrink_bisimulation(max_states=50000, max_states_before_merge=50000, "
        "threshold=1, greedy=false)");
    ValueExplanations shrink_value_explanations;
    shrink_value_explanations.push_back(
        make_pair("shrink_fh(max_states=N)",
                  "f-preserving transition systems from the "
                  "Helmert/Haslum/Hoffmann ICAPS 2007 paper "
                  "(called HHH in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Here, N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the default linear merge strategy "
                  "CG_GOAL_LEVEL to match the heuristic "
                  "in the paper. "
                  "This strategy performs best when used with label reduction "
                  "before merging, it is hence recommended to set "
                  "reduce_labels_before_shrinking=false and "
                  "reduce_labels_before_merging=true."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=infinity, threshold=1, greedy=true)",
                  "Greedy bisimulation without size bound "
                  "(called M&S-gop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
                  "the heuristic in the paper. "
                  "This strategy performs best when used with label reduction "
                  "before shrinking, it is hence recommended to set "
                  "reduce_labels_before_shrinking=true and "
                  "reduce_labels_before_merging=false."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=N, greedy=false)",
                  "Exact bisimulation with a size limit "
                  "(called DFP-bop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert), "
                  "where N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
                  "the heuristic in the paper. "
                  "This strategy performs best when used with label reduction "
                  "before shrinking, it is hence recommended to set "
                  "reduce_labels_before_shrinking=true and "
                  "reduce_labels_before_merging=false."));
    parser.document_values("shrink_strategy", shrink_value_explanations);

    // Label reduction option.
    parser.add_option<Labels *>("label_reduction",
                                "Choose relevant options for label reduction. "
                                "Also note the interaction with shrink strategies.");

    // General merge-and-shrink options.
    parser.add_option<bool>(
        "expensive_statistics",
        "show statistics on \"unique unlabeled edges\" (WARNING: "
        "these are *very* slow, i.e. too expensive to show by default "
        "(in terms of time and memory). When this is used, the planner "
        "prints a big warning on stderr with information on the performance "
        "impact. Don't use when benchmarking!)",
        "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run()) {
        return 0;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
