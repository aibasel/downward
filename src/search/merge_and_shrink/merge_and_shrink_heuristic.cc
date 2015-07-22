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
      merge_strategy(opts.get<shared_ptr<MergeStrategy> >("merge_strategy")),
      shrink_strategy(opts.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      labels(opts.get<shared_ptr<Labels>>("label_reduction")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")),
      starting_peak_memory(-1),
      final_transition_system(nullptr) {
    /*
      TODO: Can we later get rid of the initialize calls, after rethinking
      how to handle communication between different components? See issue559.
    */
    merge_strategy->initialize(task);
    labels->initialize(task_proxy);
}

void MergeAndShrinkHeuristic::report_peak_memory_delta(bool final) const {
    if (final)
        cout << "Final";
    else
        cout << "Current";
    cout << " peak memory increase of merge-and-shrink computation: "
         << get_peak_memory_in_kb() - starting_peak_memory << " KB" << endl;
}

void MergeAndShrinkHeuristic::dump_options() const {
    merge_strategy->dump_options();
    shrink_strategy->dump_options();
    labels->dump_options();
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
        "When last tested (around Subversion revision 3011), enabling "
        "the extra statistics\nincreased heuristic generation time by "
        "76%. This figure may be significantly\nworse with more "
        "recent code or for particular domains and instances.\n"
        "You have been warned. Don't use this for benchmarking!"
        << endl << dashes << endl;
    }
    if (!labels->reduce_before_merging() && !labels->reduce_before_shrinking()) {
        cerr << dashes << endl
             << "WARNING! You did not enable label reduction. This may "
        "drastically reduce the performance of merge-and-shrink!"
             << endl << dashes << endl;
    } else if (labels->reduce_before_merging() && labels->reduce_before_shrinking()) {
        cerr << dashes << endl
             << "WARNING! You set label reduction to be applied twice in "
        "each merge-and-shrink iteration, both before shrinking and\n"
        "merging. This double computation effort does not pay off "
        "for most configurations!"
        << endl << dashes << endl;
    } else {
        if (labels->reduce_before_shrinking() &&
            (shrink_strategy->get_name() == "f-preserving"
             || shrink_strategy->get_name() == "random")) {
            cerr << dashes << endl
                 << "WARNING! Bucket-based shrink strategies such as "
            "f-preserving random perform best if used with label\n"
            "reduction before merging, not before shrinking!"
            << endl << dashes << endl;
        }
        if (labels->reduce_before_merging() &&
            shrink_strategy->get_name() == "bisimulation") {
            cerr << dashes << endl
                 << "WARNING! Shrinking based on bisimulation performs best "
            "if used with label reduction before shrinking, not\n"
            "before merging!"
            << endl << dashes << endl;
        }
    }
}

void MergeAndShrinkHeuristic::build_transition_system(const Timer &timer) {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_transition_systems also
    //       allocates memory.

    // Set of all transition systems. Entries with nullptr have been merged.
    vector<TransitionSystem *> all_transition_systems;
    size_t num_vars = task_proxy.get_variables().size();
    all_transition_systems.reserve(num_vars * 2 - 1);
    TransitionSystem::build_atomic_transition_systems(
        task_proxy, all_transition_systems, labels);
    for (TransitionSystem *transition_system : all_transition_systems) {
        if (!transition_system->is_solvable()) {
            final_transition_system = transition_system;
            break;
        }
    }
    cout << endl;

    if (!final_transition_system) { // All atomic transition system are solvable.
        while (!merge_strategy->done()) {
            // Choose next transition systems to merge
            pair<int, int> merge_indices = merge_strategy->get_next(all_transition_systems);
            int merge_index1 = merge_indices.first;
            int merge_index2 = merge_indices.second;
            assert(merge_index1 != merge_index2);
            TransitionSystem *transition_system1 = all_transition_systems[merge_index1];
            TransitionSystem *transition_system2 = all_transition_systems[merge_index2];
            assert(transition_system1);
            assert(transition_system2);
            transition_system1->statistics(timer, use_expensive_statistics);
            transition_system2->statistics(timer, use_expensive_statistics);

            if (labels->reduce_before_shrinking()) {
                labels->reduce(merge_indices, all_transition_systems);
            }

            // Shrinking
            pair<bool, bool> shrunk = shrink_strategy->shrink(
                *transition_system1, *transition_system2);
            if (shrunk.first)
                transition_system1->statistics(timer, use_expensive_statistics);
            if (shrunk.second)
                transition_system2->statistics(timer, use_expensive_statistics);

            if (labels->reduce_before_merging()) {
                labels->reduce(merge_indices, all_transition_systems);
            }

            // Merging
            TransitionSystem *new_transition_system = new CompositeTransitionSystem(
                task_proxy, labels, transition_system1, transition_system2);
            new_transition_system->statistics(timer, use_expensive_statistics);
            all_transition_systems.push_back(new_transition_system);

            /*
              NOTE: both the shrinking strategy classes and the construction of
              the composite require input transition systems to be solvable.
            */
            if (!new_transition_system->is_solvable()) {
                final_transition_system = new_transition_system;
                break;
            }

            transition_system1->release_memory();
            transition_system2->release_memory();
            all_transition_systems[merge_index1] = nullptr;
            all_transition_systems[merge_index2] = nullptr;

            report_peak_memory_delta();
            cout << endl;
        }
    }

    if (final_transition_system) {
        // Problem is unsolvable
        for (TransitionSystem *transition_system : all_transition_systems) {
            if (transition_system) {
                transition_system->release_memory();
                transition_system = nullptr;
            }
        }
    } else {
        assert(all_transition_systems.size() == num_vars * 2 - 1);
        for (size_t i = 0; i < all_transition_systems.size() - 1; ++i) {
            assert(!all_transition_systems[i]);
        }
        final_transition_system = all_transition_systems.back();
        assert(final_transition_system);
        final_transition_system->release_memory();
    }

    labels = nullptr;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    starting_peak_memory = get_peak_memory_in_kb();
    verify_no_axioms(task_proxy);
    dump_options();
    warn_on_unusual_options();
    cout << endl;

    build_transition_system(timer);
    if (final_transition_system->is_solvable()) {
        cout << "Final transition system size: "
             << final_transition_system->get_size() << endl;
        // TODO: after adopting the task interface everywhere, change this
        // back to compute_heuristic(task_proxy.get_initial_state())
        cout << "initial h value: "
             << final_transition_system->get_cost(task_proxy.get_initial_state())
             << endl;
    } else {
        cout << "Abstract problem is unsolvable!" << endl;
    }
    report_peak_memory_delta(true);
    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl;
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
    parser.document_synopsis("Merge-and-shrink heuristic",
                             "A currently recommended good configuration uses bisimulation "
                             "based shrinking (alternating max states from 50000 to 200000 is "
                             "reasonable), DFP merging, and the appropriate label "
                             "reduction setting:\n"
                             "merge_and_shrink(shrink_strategy=shrink_bisimulation(max_states=100000,"
                             "threshold=1,greedy=false),merge_strategy=merge_dfp,"
                             "label_reduction=label_reduction(before_shrinking=true, before_merging=false))");
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
    parser.add_option<shared_ptr<MergeStrategy> >(
        "merge_strategy",
        "merge strategy; choose between merge_linear with various variable "
        "orderings and merge_dfp.");

    // Shrink strategy option.
    parser.add_option<shared_ptr<ShrinkStrategy> >(
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
    parser.add_option<shared_ptr<Labels>>("label_reduction",
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
        return nullptr;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
