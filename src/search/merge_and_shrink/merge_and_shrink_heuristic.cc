#include "merge_and_shrink_heuristic.h"

#include "factored_transition_system.h"
#include "fts_factory.h"
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
      shrink_strategy(opts.get<shared_ptr<ShrinkStrategy> >("shrink_strategy")),
      labels(opts.get<shared_ptr<Labels> >("label_reduction")),
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
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    string dashes(79, '=');
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

    FactoredTransitionSystem fts = create_factored_transition_system(
        task_proxy, labels);
    for (TransitionSystem *transition_system : fts.get_vector()) {
        if (!transition_system->is_solvable()) {
            final_transition_system = transition_system;
            break;
        }
    }
    cout << endl;

    if (!final_transition_system) { // All atomic transition system are solvable.
        while (!merge_strategy->done()) {
            // Choose next transition systems to merge
            pair<int, int> merge_indices = merge_strategy->get_next(
                fts.get_vector());
            int merge_index1 = merge_indices.first;
            int merge_index2 = merge_indices.second;
            assert(merge_index1 != merge_index2);
            TransitionSystem *transition_system1 = fts[merge_index1];
            TransitionSystem *transition_system2 = fts[merge_index2];
            assert(transition_system1);
            assert(transition_system2);
            transition_system1->statistics(timer);
            transition_system2->statistics(timer);

            if (labels->reduce_before_shrinking()) {
                labels->reduce(merge_indices, fts.get_vector());
            }

            // Shrinking
            pair<bool, bool> shrunk = shrink_strategy->shrink(
                *transition_system1, *transition_system2);
            if (shrunk.first)
                transition_system1->statistics(timer);
            if (shrunk.second)
                transition_system2->statistics(timer);

            if (labels->reduce_before_merging()) {
                labels->reduce(merge_indices, fts.get_vector());
            }

            // Merging
            TransitionSystem *new_transition_system = new TransitionSystem(
                task_proxy, labels, transition_system1, transition_system2);
            new_transition_system->statistics(timer);
            fts.get_vector().push_back(new_transition_system);

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
            fts[merge_index1] = nullptr;
            fts[merge_index2] = nullptr;

            report_peak_memory_delta();
            cout << endl;
        }
    }

    if (final_transition_system) {
        // Problem is unsolvable
        for (TransitionSystem *transition_system : fts.get_vector()) {
            if (transition_system) {
                transition_system->release_memory();
                transition_system = nullptr;
            }
        }
    } else {
        assert(fts.get_vector().size() ==
               task_proxy.get_variables().size() * 2 - 1);
        for (size_t i = 0; i < fts.get_vector().size() - 1; ++i) {
            assert(!fts[i]);
        }
        final_transition_system = fts.get_vector().back();
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
    parser.document_synopsis(
        "Merge-and-shrink heuristic",
        "This heuristic implements the algorithm described in the following "
        "paper:\n\n"
        " * Silvan Sievers, Martin Wehrle, and Malte Helmert.<<BR>>\n"
        " [Generalized Label Reduction for Merge-and-Shrink Heuristics "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf].<<BR>>\n "
        "In //Proceedings of the 28th AAAI Conference on Artificial "
        "Intelligence (AAAI 2014)//, pp. 2358-2366. AAAI Press 2014.\n"
        "For a more exhaustive description of merge-and-shrink, see the journal "
        "paper\n\n"
        " * Malte Helmert, Patrik Haslum, Joerg Hoffmann, and Raz Nissim.<<BR>>\n"
        " [Merge-and-Shrink Abstraction: A Method for Generating Lower Bounds "
        "in Factored State Spaces "
        "http://ai.cs.unibas.ch/papers/helmert-et-al-jacm2014.pdf].<<BR>>\n "
        "//Journal of the ACM 61 (3)//, pp. 16:1-63. 2014\n"
        "Please note that the journal paper describes the \"old\" theory of "
        "label reduction, which has been superseded by the above conference "
        "paper and is no longer implemented in Fast Downward.");
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
        "which can lead to poor heuristics even when only perfect shrinking "
        "is performed.");
    parser.document_note(
        "Note",
        "A currently recommended good configuration uses bisimulation "
        "based shrinking (selecting max states from 50000 to 200000 is "
        "reasonable), DFP merging, and the appropriate label "
        "reduction setting:\n"
        "merge_and_shrink(shrink_strategy=shrink_bisimulation(max_states=100000,"
        "threshold=1,greedy=false),merge_strategy=merge_dfp(),"
        "label_reduction=label_reduction(before_shrinking=true, before_merging=false))");

    // Merge strategy option.
    parser.add_option<shared_ptr<MergeStrategy> >(
        "merge_strategy",
        "See detailed documentation for merge strategies. "
        "We currently recommend merge_dfp.");

    // Shrink strategy option.
    parser.add_option<shared_ptr<ShrinkStrategy> >(
        "shrink_strategy",
        "See detailed documentation for shrink strategies. "
        "We currently recommend shrink_bisimulation.");

    // Label reduction option.
    parser.add_option<shared_ptr<Labels> >(
        "label_reduction",
        "See detailed documentation for labels. There is currently only "
        "one 'option' to use label_reduction. Also note the interaction "
        "with shrink strategies.");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    } else {
        return new MergeAndShrinkHeuristic(opts);
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
