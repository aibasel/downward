#include "merge_and_shrink_heuristic.h"

#include "abstraction.h"
#include "labels.h"
#include "merge_strategy.h"
#include "shrink_fh.h"
#include "shrink_strategy.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"

#include <cassert>
#include <vector>
using namespace std;


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      merge_strategy(opts.get<MergeStrategy *>("merge_strategy")),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
    labels = new Labels(cost_type, LabelReduction(opts.get_enum("label_reduction")),
                        FixpointVariableOrder(opts.get_enum("fixpoint_var_order")));
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
    delete merge_strategy;
    delete labels;
}

void MergeAndShrinkHeuristic::dump_options() const {
    merge_strategy->dump_options();
    shrink_strategy->dump_options();
    labels->dump_options();
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

Abstraction *MergeAndShrinkHeuristic::build_abstraction() {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_abstractions also
    //       allocates memory.

    vector<Abstraction *> atomic_abstractions;
    Abstraction::build_atomic_abstractions(
        is_unit_cost_problem(), atomic_abstractions, labels);
    // vector of all abstractions. entries with 0 have been merged.
    vector<Abstraction *> all_abstractions(atomic_abstractions);

    cout << "Shrinking atomic abstractions..." << endl;
    for (size_t i = 0; i < atomic_abstractions.size(); ++i) {
        assert(atomic_abstractions[i]->sorted_unique());
        atomic_abstractions[i]->compute_distances();
        if (!atomic_abstractions[i]->is_solvable())
            return atomic_abstractions[i];
        shrink_strategy->shrink_atomic(*atomic_abstractions[i]);
    }

    cout << "Merging abstractions..." << endl;

    // TODO: fix case where there is only one variable
    // remove variable "abstraction"
    Abstraction *abstraction = 0;
    int total_reduced_labels = 0;
    vector<int> system_order;
    // TODO: reconsider in which order things are done in the main loop
    while (!merge_strategy->done()) {
        pair<int, int> next_systems;
        merge_strategy->get_next(all_abstractions, next_systems);
        int system_one = next_systems.first;
        system_order.push_back(system_one);
        abstraction = all_abstractions[system_one];
        assert(abstraction);
        int system_two = next_systems.second;
        assert(system_one != system_two);
        system_order.push_back(system_two);
        Abstraction *other_abstraction = all_abstractions[system_two];
        assert(other_abstraction);

        // TODO: When using nonlinear merge strategies, make sure not
        // to normalize multiple parts of a composite. See issue68.
        // Note: do not reduce labels several times for the same abstraction!
        bool reduced_labels = false;
        if (shrink_strategy->reduce_labels_before_shrinking()) {
            total_reduced_labels += labels->reduce(system_one, all_abstractions);
            reduced_labels = true;
            abstraction->normalize();
            other_abstraction->normalize();
            abstraction->statistics(use_expensive_statistics);
            other_abstraction->statistics(use_expensive_statistics);
        }

        abstraction->compute_distances();
        // TODO: check for which abstraction?
        if (!abstraction->is_solvable())
            return abstraction;

        other_abstraction->compute_distances();
        shrink_strategy->shrink_before_merge(*abstraction, *other_abstraction);
        // TODO: Make shrink_before_merge return a pair of bools
        //       that tells us whether they have actually changed,
        //       and use that to decide whether to dump statistics?
        // (The old code would print statistics on abstraction iff it was
        // shrunk. This is not so easy any more since this method is not
        // in control, and the shrink strategy doesn't know whether we want
        // expensive statistics. As a temporary aid, we just print the
        // statistics always now, whether or not we shrunk.)
        abstraction->statistics(use_expensive_statistics);
        other_abstraction->statistics(use_expensive_statistics);

        if (!reduced_labels) {
            total_reduced_labels += labels->reduce(system_one, all_abstractions);
        }
        abstraction->normalize();
        other_abstraction->normalize();
        if (!reduced_labels) {
            // only print statistics if we just possibly reduced labels
            other_abstraction->statistics(use_expensive_statistics);
            abstraction->statistics(use_expensive_statistics);
        }

        Abstraction *new_abstraction = new CompositeAbstraction(
            is_unit_cost_problem(),
            labels,
            abstraction, other_abstraction);

        abstraction->release_memory();
        other_abstraction->release_memory();

        abstraction = new_abstraction;
        abstraction->statistics(use_expensive_statistics);

        cout << "Number of reduced labels so far: " << total_reduced_labels << endl;

        all_abstractions[system_one] = abstraction;
        all_abstractions[system_two] = 0;
    }

    // TODO: iterate over all_abstraction and assert that only one abstraction
    // is left over. Otherwise throw exception.

    abstraction->compute_distances();
    if (!abstraction->is_solvable())
        return abstraction;

    ShrinkStrategy *def_shrink = ShrinkFH::create_default(abstraction->size());
    def_shrink->shrink(*abstraction, abstraction->size(), true);
    abstraction->compute_distances();

    abstraction->statistics(use_expensive_statistics);
    abstraction->release_memory();

    cout << "Final number of reduced labels: " << total_reduced_labels << endl;
    cout << "Order of merged indices: ";
    for (size_t i = 1; i < system_order.size(); i += 2) {
        cout << system_order[i - 1] << " " << system_order[i] << ", ";
    }
    cout << endl;
    merge_strategy->print_summary();
    return abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms_no_cond_effects();

    cout << "Building abstraction..." << endl;
    final_abstraction = build_abstraction();
    if (!final_abstraction->is_solvable()) {
        cout << "Abstract problem is unsolvable!" << endl;
    }

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl << "initial h value: " << compute_heuristic(g_initial_state()) << endl;
    cout << "Estimated peak memory for abstraction: " << final_abstraction->get_peak_memory_estimate() << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const State &state) {
    int cost = final_abstraction->get_cost(state);
    if (cost == -1)
        return DEAD_END;
    return cost;
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Merge-and-shrink heuristic", "");
    parser.document_language_support(
        "action costs",
        "supported");
    parser.document_language_support("conditional_effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_option<MergeStrategy *>(
        "merge_strategy",
        "merge strategy; choose between merge_linear or merge_non_linear",
        "merge_linear");

    parser.add_option<ShrinkStrategy *>(
        "shrink_strategy",
        "shrink strategy; these are not fully documented yet; "
        "try one of the following:",
        "shrink_fh(max_states=50000, max_states_before_merge=50000, shrink_f=high, shrink_h=low)");
    ValueExplanations shrink_value_explanations;
    shrink_value_explanations.push_back(
        make_pair("shrink_fh(max_states=N)",
                  "f-preserving abstractions from the "
                  "Helmert/Haslum/Hoffmann ICAPS 2007 paper "
                  "(called HHH in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Here, N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the default merge strategy "
                  "MERGE_LINEAR_CG_GOAL_LEVEL to match the heuristic "
                  "in the paper."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=infinity, threshold=1, greedy=true, initialize_by_h=false, group_by_h=false)",
                  "Greedy bisimulation without size bound "
                  "(called M&S-gop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Combine this with the merge strategy "
                  "MERGE_LINEAR_REVERSE_LEVEL to match "
                  "the heuristic in the paper. "));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=N, greedy=false, initialize_by_h=true, group_by_h=true)",
                  "Exact bisimulation with a size limit "
                  "(called DFP-bop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert), "
                  "where N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the merge strategy "
                  "MERGE_LINEAR_REVERSE_LEVEL to match "
                  "the heuristic in the paper."));
    parser.document_values("shrink_strategy", shrink_value_explanations);

    vector<string> label_reduction;
    label_reduction.push_back("NONE");
    label_reduction.push_back("OLD");
    label_reduction.push_back("APPROXIMATIVE");
    label_reduction.push_back("APPROXIMATIVE_WITH_FIXPOINT");
    label_reduction.push_back("EXACT");
    label_reduction.push_back("EXACT_WITH_FIXPOINT");
    parser.add_enum_option("label_reduction", label_reduction, "label reduction method", "EXACT_WITH_FIXPOINT");
    vector<string> fixpoint_variable_order;
    fixpoint_variable_order.push_back("REGULAR");
    fixpoint_variable_order.push_back("REVERSE");
    fixpoint_variable_order.push_back("RANDOM");
    parser.add_enum_option("fixpoint_var_order", fixpoint_variable_order,
                           "order in which variables are considered when using "
                           "fixpoint iteration for label reduction", "REGULAR");
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
        if (opts.get_enum("label_reduction") == OLD
                && opts.get<MergeStrategy *>("merge_strategy")->name() != "linear") {
            parser.error("old label reduction is only correct when used with a "
                         "linear merge strategy!");
        }
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<Heuristic> _plugin("merge_and_shrink", _parse);
