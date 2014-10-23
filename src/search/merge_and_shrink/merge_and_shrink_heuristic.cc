#include "merge_and_shrink_heuristic.h"

#include "transition_system.h"
#include "labels.h"
#include "merge_strategy.h"
#include "shrink_strategy.h"

#include "../global_state.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../timer.h"

#include <cassert>
#include <vector>
using namespace std;

MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      merge_strategy(opts.get<MergeStrategy *>("merge_strategy")),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
    labels = new Labels(opts);
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
    delete merge_strategy;
    delete shrink_strategy;
    delete labels;
}

void MergeAndShrinkHeuristic::dump_options() const {
    merge_strategy->dump_options();
    shrink_strategy->dump_options();
    labels->dump_label_reduction_options();
    cout << "Expensive statistics: "
         << (use_expensive_statistics ? "enabled" : "disabled") << endl;
}

void MergeAndShrinkHeuristic::warn_on_unusual_options() const {
    if (use_expensive_statistics) {
        string dashes(79, '=');
        cerr << dashes << endl
             << ("WARNING! You have enabled extra statistics for "
            "merge-and-shrink heuristics.\n"
            "These statistics require a lot of time and memory.\n"
            "When last tested (around revision 3011), enabling the "
            "extra statistics\nincreased heuristic generation time by "
            "76%. This figure may be significantly\nworse with more "
            "recent code or for particular domains and instances.\n"
            "You have been warned. Don't use this for benchmarking!")
        << endl << dashes << endl;
    }
}

TransitionSystem *MergeAndShrinkHeuristic::build_transition_system() {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_transition_systems also
    //       allocates memory.

    // Set of all transition systems. Entries with 0 have been merged.
    vector<TransitionSystem *> all_transition_systems;
    if (g_variable_domain.size() * 2 - 1 > all_transition_systems.max_size())
        exit_with(EXIT_OUT_OF_MEMORY);
    all_transition_systems.reserve(g_variable_domain.size() * 2 - 1);
    TransitionSystem::build_atomic_transition_systems(all_transition_systems, labels, cost_type);

    cout << "Merging transition systems..." << endl;

    vector<int> transition_system_order;
    while (!merge_strategy->done()) {
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

        // Note: we do not reduce labels several times for the same transition system
        bool reduced_labels = false;
        if (shrink_strategy->reduce_labels_before_shrinking()) {
            labels->reduce(make_pair(system_one, system_two), all_transition_systems);
            reduced_labels = true;
            transition_system->statistics(use_expensive_statistics);
            other_transition_system->statistics(use_expensive_statistics);
        }

        if (!transition_system->is_solvable())
            return transition_system;
        if (!other_transition_system->is_solvable())
            return other_transition_system;

        shrink_strategy->shrink_before_merge(*transition_system, *other_transition_system);
        // TODO: Make shrink_before_merge return a pair of bools
        //       that tells us whether they have actually changed,
        //       and use that to decide whether to dump statistics?
        // (The old code would print statistics on transition system iff it was
        // shrunk. This is not so easy any more since this method is not
        // in control, and the shrink strategy doesn't know whether we want
        // expensive statistics. As a temporary aid, we just print the
        // statistics always now, whether or not we shrunk.)
        transition_system->statistics(use_expensive_statistics);
        other_transition_system->statistics(use_expensive_statistics);

        if (!reduced_labels) {
            labels->reduce(make_pair(system_one, system_two), all_transition_systems);
        }
        if (!reduced_labels) {
            // only print statistics if we just possibly reduced labels
            other_transition_system->statistics(use_expensive_statistics);
            transition_system->statistics(use_expensive_statistics);
        }

        TransitionSystem *new_transition_system = new CompositeTransitionSystem(
            labels, transition_system, other_transition_system);

        transition_system->release_memory();
        other_transition_system->release_memory();

        new_transition_system->statistics(use_expensive_statistics);

        all_transition_systems[system_one] = 0;
        all_transition_systems[system_two] = 0;
        all_transition_systems.push_back(new_transition_system);
    }

    assert(all_transition_systems.size() == g_variable_domain.size() * 2 - 1);
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

    final_transition_system->statistics(use_expensive_statistics);
    final_transition_system->release_memory();

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
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms();

    cout << "Building transition system..." << endl;
    final_transition_system = build_transition_system();
    if (!final_transition_system->is_solvable()) {
        cout << "Abstract problem is unsolvable!" << endl;
    }

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl << "initial h value: " << compute_heuristic(g_initial_state()) << endl;
    cout << "Estimated peak memory for transition system: " << final_transition_system->get_peak_memory_estimate() << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const GlobalState &state) {
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

    parser.add_option<MergeStrategy *>(
        "merge_strategy",
        "merge strategy; choose between merge_linear and merge_dfp",
        "merge_linear");

    parser.add_option<ShrinkStrategy *>(
        "shrink_strategy",
        "shrink strategy; "
        "try one of the following:",
        "shrink_fh(max_states=50000, max_states_before_merge=50000, shrink_f=high, shrink_h=low)");
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
                  "in the paper."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=infinity, threshold=1, greedy=true, group_by_h=false)",
                  "Greedy bisimulation without size bound "
                  "(called M&S-gop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
                  "the heuristic in the paper. "));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=N, greedy=false, group_by_h=true)",
                  "Exact bisimulation with a size limit "
                  "(called DFP-bop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert), "
                  "where N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
                  "the heuristic in the paper."));
    parser.document_values("shrink_strategy", shrink_value_explanations);

    vector<string> label_reduction_method;
    vector<string> label_reduction_method_doc;
    label_reduction_method.push_back("NONE");
    label_reduction_method_doc.push_back(
        "no label reduction will be performed");
    label_reduction_method.push_back("TWO_TRANSITION_SYSTEMS");
    label_reduction_method_doc.push_back(
        "compute the 'combinable relation' only for the two transition "
        "systems being merged next");
    label_reduction_method.push_back("ALL_TRANSITION_SYSTEMS");
    label_reduction_method_doc.push_back(
        "compute the 'combinable relation' for labels once for every "
        "transition  system and reduce labels");
    label_reduction_method.push_back("ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT");
    label_reduction_method_doc.push_back(
        "keep computing the 'combinable relation' for labels iteratively "
        "for all transition systems until no more labels can be reduced");
    parser.add_enum_option("label_reduction_method",
                           label_reduction_method,
                           "Label reduction method. See the AAAI14 by Sievers "
                           "et al. for explanation of the default label "
                           "reduction method and the 'combinable relation'.",
                           "ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT",
                           label_reduction_method_doc);

    vector<string> label_reduction_system_order;
    vector<string> label_reduction_system_order_doc;
    label_reduction_system_order.push_back("REGULAR");
    label_reduction_system_order_doc.push_back(
        "transition systems are considered in the FD given order for "
        "atomic transition systems and in the order of their creation "
        "for composite transition system.");
    label_reduction_system_order.push_back("REVERSE");
    label_reduction_system_order_doc.push_back(
        "inverse of REGULAR");
    label_reduction_system_order.push_back("RANDOM");
    label_reduction_system_order_doc.push_back(
        "random order");
    parser.add_enum_option("label_reduction_system_order",
                           label_reduction_system_order,
                           "Order of transition systems for the label reduction "
                           "methods that iterate over the set of all transition "
                           "systems. Only useful for the choices "
                           "all_transition_systems and "
                           "all_transition_systems_with_fixpoint for the option "
                           "label_reduction_method.",
                           "RANDOM",
                           label_reduction_system_order_doc);

    parser.add_option<bool>("expensive_statistics",
                            "show statistics on \"unique unlabeled edges\" (WARNING: "
                            "these are *very* slow, i.e. too expensive to show by default "
                            "(in terms of time and memory). When this is used, the planner "
                            "prints a big warning on stderr with information on the performance impact. "
                            "Don't use when benchmarking!)",
                            "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (parser.dry_run()) {
        return 0;
    } else {
        if (opts.get_enum("label_reduction_method") == 1
            && opts.get<MergeStrategy *>("merge_strategy")->name() != "linear") {
            parser.error("old label reduction is only correct when used with a "
                         "linear merge strategy!");
        }
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
