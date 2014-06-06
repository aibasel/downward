#include "merge_and_shrink_heuristic.h"

#include "abstraction.h"
#include "labels.h"
#include "merge_strategy.h"
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
    labels = new Labels(is_unit_cost_problem(), opts, cost_type);
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
    all_abstractions.reserve(g_variable_domain.size() * 2 - 1);
    Abstraction::build_atomic_abstractions(all_abstractions, labels);

    cout << "Shrinking atomic abstractions..." << endl;
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        all_abstractions[i]->compute_distances();
        if (!all_abstractions[i]->is_solvable())
            return all_abstractions[i];
        shrink_strategy->shrink_atomic(*all_abstractions[i]);
    }

    cout << "Merging abstractions..." << endl;

    vector<int> system_order;
    while (!merge_strategy->done()) {
        pair<int, int> next_systems = merge_strategy->get_next(all_abstractions);
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
            labels->reduce(make_pair(system_one, system_two), all_abstractions);
            reduced_labels = true;
            abstraction->normalize();
            other_abstraction->normalize();
            abstraction->statistics(use_expensive_statistics);
            other_abstraction->statistics(use_expensive_statistics);
        }

        // distances need to be computed before shrinking
        abstraction->compute_distances();
        other_abstraction->compute_distances();
        if (!abstraction->is_solvable())
            return abstraction;
        if (!other_abstraction->is_solvable())
            return other_abstraction;

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
            labels->reduce(make_pair(system_one, system_two), all_abstractions);
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

        all_abstractions[system_one] = 0;
        all_abstractions[system_two] = 0;
        all_abstractions.push_back(new_abstraction);
    }

    assert(all_abstractions.size() == g_variable_domain.size() * 2 - 1);
    Abstraction *final_abstraction = 0;
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        if (all_abstractions[i]) {
            if (final_abstraction) {
                cerr << "Found more than one remaining abstraction!" << endl;
                exit_with(EXIT_CRITICAL_ERROR);
            }
            final_abstraction = all_abstractions[i];
            assert(i == all_abstractions.size() - 1);
        }
    }

    final_abstraction->compute_distances();
    if (!final_abstraction->is_solvable())
        return final_abstraction;

    final_abstraction->statistics(use_expensive_statistics);
    final_abstraction->release_memory();

    cout << "Order of merged systems: ";
    for (size_t i = 1; i < system_order.size(); i += 2) {
        cout << system_order[i - 1] << " " << system_order[i] << ", ";
    }
    cout << endl;
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
        "merge-and-shrink paper), the atomic abstractions on which "
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

    vector<string> label_reduction_method;
    label_reduction_method.push_back("NONE");
    label_reduction_method.push_back("OLD");
    label_reduction_method.push_back("TWO_ABSTRACTIONS");
    label_reduction_method.push_back("ALL_ABSTRACTIONS");
    label_reduction_method.push_back("ALL_ABSTRACTIONS_WITH_FIXPOINT");
    parser.add_enum_option("label_reduction_method", label_reduction_method,
                           "label reduction method: "
                           "none: no label reduction will be performed "
                           "old: emulate the label reduction as desribed in the "
                           "IJCAI 2011 paper by Nissim, Hoffmann and Helmert."
                           "two_abstractions: compute the 'combinable relation' "
                           "for labels only for the two abstractions that will "
                           "be merged next and reduce labels."
                           "all_abstractions: compute the 'combinable relation' "
                           "for labels once for every abstraction and reduce "
                           "labels."
                           "all_abstractions_with_fixpoint: keep computing the "
                           "'combinable relation' for labels iteratively for all "
                           "abstractions until no more labels can be reduced.",
                           "ALL_ABSTRACTIONS_WITH_FIXPOINT");
    vector<string> label_reduction_system_order;
    label_reduction_system_order.push_back("REGULAR");
    label_reduction_system_order.push_back("REVERSE");
    label_reduction_system_order.push_back("RANDOM");
    parser.add_enum_option("label_reduction_system_order", label_reduction_system_order,
                           "order of transition systems for the label reduction methods "
                           "that iterate over the set of all abstractions. only useful "
                           "for the choices all_abstractions and all_abstractions_with_fixpoint "
                           "for the option label_reduction_method.", "RANDOM");
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
