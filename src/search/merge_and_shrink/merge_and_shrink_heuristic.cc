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
    labels = new Labels(cost_type,
                        is_unit_cost_problem(),
                        LabelReduction(opts.get_enum("label_reduction")),
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

    // vector of all abstractions. entries with 0 have been merged.
    vector<Abstraction *> all_abstractions;
    all_abstractions.reserve(g_variable_domain.size());
    Abstraction::build_atomic_abstractions(all_abstractions, labels);

    cout << "Shrinking atomic abstractions..." << endl;
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        assert(all_abstractions[i]->sorted_unique());
        all_abstractions[i]->compute_distances();
        if (!all_abstractions[i]->is_solvable())
            return all_abstractions[i];
        shrink_strategy->shrink_atomic(*all_abstractions[i]);
    }

    cout << "Merging abstractions..." << endl;

    // TODO: fix case where there is only one variable
    vector<int> system_order;
    // TODO: reconsider in which order things are done in the main loop
    while (!merge_strategy->done()) {
        pair<int, int> next_systems;
        merge_strategy->get_next(all_abstractions, next_systems);
        int system_one = next_systems.first;
        system_order.push_back(system_one);
        Abstraction *abstraction = all_abstractions[system_one];
        assert(abstraction);
        int system_two = next_systems.second;
        assert(system_one != system_two);
        system_order.push_back(system_two);
        Abstraction *other_abstraction = all_abstractions[system_two];
        assert(other_abstraction);

        // Note: we do not reduce labels several times for the same abstraction
        bool reduced_labels = false;
        if (shrink_strategy->reduce_labels_before_shrinking()) {
            labels->reduce(system_one, all_abstractions);
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
            labels->reduce(system_one, all_abstractions);
        }
        abstraction->normalize();
        other_abstraction->normalize();
        if (!reduced_labels) {
            // only print statistics if we just possibly reduced labels
            other_abstraction->statistics(use_expensive_statistics);
            abstraction->statistics(use_expensive_statistics);
        }

        Abstraction *new_abstraction = new CompositeAbstraction(labels,
                                                                abstraction,
                                                                other_abstraction);

        abstraction->release_memory();
        other_abstraction->release_memory();

        new_abstraction->statistics(use_expensive_statistics);

        all_abstractions[system_one] = new_abstraction;
        all_abstractions[system_two] = 0;
    }

    Abstraction *final_abstraction = 0;
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        if (all_abstractions[i]) {
            if (final_abstraction) {
                cerr << "Found more than one remaining abstration!" << endl;
                exit_with(EXIT_CRITICAL_ERROR);
            }
            final_abstraction = all_abstractions[i];
        }
    }

    final_abstraction->compute_distances();
    if (!final_abstraction->is_solvable())
        return final_abstraction;

    // TODO: get rid of this block (unless it does something meaningful)
    ShrinkStrategy *def_shrink = ShrinkFH::create_default(final_abstraction->size());
    def_shrink->shrink(*final_abstraction, final_abstraction->size(), true);
    final_abstraction->compute_distances();

    final_abstraction->statistics(use_expensive_statistics);
    final_abstraction->release_memory();

    cout << "Order of merged indices: ";
    for (size_t i = 1; i < system_order.size(); i += 2) {
        cout << system_order[i - 1] << " " << system_order[i] << ", ";
    }
    cout << endl;
    merge_strategy->print_summary();
    return final_abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms();

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
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional_effects", "supported (but see note)");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");
    parser.document_note(
        "Note",
        "Conditional effects are supported directly. Note, however, that "
        "for tasks that are not factored (in the sense of the JACM 2014 "
        "merge-and-shrink paper), the atomic abstractions on which "
        "merge-and-shrink heuristics are based are nondeterministic, "
        "which can lead to poor heuristics even when no shrinking is "
        "performed.");

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
                  "Combine this with the default linear merge strategy "
                  "CG_GOAL_LEVEL to match the heuristic "
                  "in the paper."));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=infinity, threshold=1, greedy=true, initialize_by_h=false, group_by_h=false)",
                  "Greedy bisimulation without size bound "
                  "(called M&S-gop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert). "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
                  "the heuristic in the paper. "));
    shrink_value_explanations.push_back(
        make_pair("shrink_bisimulation(max_states=N, greedy=false, initialize_by_h=true, group_by_h=true)",
                  "Exact bisimulation with a size limit "
                  "(called DFP-bop in the IJCAI 2011 paper by Nissim, "
                  "Hoffmann and Helmert), "
                  "where N is a numerical parameter for which sensible values "
                  "include 1000, 10000, 50000, 100000 and 200000. "
                  "Combine this with the linear merge strategy "
                  "REVERSE_LEVEL to match "
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
