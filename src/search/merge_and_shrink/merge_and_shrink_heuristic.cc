#include "merge_and_shrink_heuristic.h"

#include "abstraction.h"
#include "shrink_fh.h"
#include "variable_order_finder.h"

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state.h"
#include "../timer.h"

#include <cassert>
#include <limits>
#include <vector>
using namespace std;


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      abstraction_count(opts.get<int>("count")),
      merge_strategy(MergeStrategy(opts.get_enum("merge_strategy"))),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      use_label_reduction(opts.get<bool>("reduce_labels")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
}

void MergeAndShrinkHeuristic::dump_options() const {
    cout << "Merge strategy: ";
    switch (merge_strategy) {
    case MERGE_LINEAR_CG_GOAL_LEVEL:
        cout << "linear CG/GOAL, tie breaking on level (main)";
        break;
    case MERGE_LINEAR_CG_GOAL_RANDOM:
        cout << "linear CG/GOAL, tie breaking random";
        break;
    case MERGE_LINEAR_GOAL_CG_LEVEL:
        cout << "linear GOAL/CG, tie breaking on level";
        break;
    case MERGE_LINEAR_RANDOM:
        cout << "linear random";
        break;
    case MERGE_DFP:
        cout << "Draeger/Finkbeiner/Podelski" << endl;
        cerr << "DFP merge strategy not implemented." << endl;
        exit_with(EXIT_UNSUPPORTED);
        break;
    case MERGE_LINEAR_LEVEL:
        cout << "linear by level";
        break;
    case MERGE_LINEAR_REVERSE_LEVEL:
        cout << "linear by reverse level";
        break;
    default:
        ABORT("Unknown merge strategy.");
    }
    cout << endl;
    shrink_strategy->dump_options();
    cout << "Number of abstractions to maximize over: "
         << abstraction_count << endl;
    cout << "Label reduction: "
         << (use_label_reduction ? "enabled" : "disabled") << endl
         << "Expensive statistics: "
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

Abstraction *MergeAndShrinkHeuristic::build_abstraction(bool is_first) {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_abstractions also
    //       allocates memory.

    vector<Abstraction *> atomic_abstractions;
    Abstraction::build_atomic_abstractions(
        is_unit_cost_problem(), get_cost_type(), atomic_abstractions);

    cout << "Shrinking atomic abstractions..." << endl;
    for (size_t i = 0; i < atomic_abstractions.size(); ++i) {
        atomic_abstractions[i]->compute_distances();
        if (!atomic_abstractions[i]->is_solvable())
            return atomic_abstractions[i];
        shrink_strategy->shrink_atomic(*atomic_abstractions[i]);
    }

    cout << "Merging abstractions..." << endl;

    VariableOrderFinder order(merge_strategy, is_first);

    int var_no = order.next();
    cout << "First variable: #" << var_no << endl;
    Abstraction *abstraction = atomic_abstractions[var_no];
    abstraction->statistics(use_expensive_statistics);

    while (!order.done()) {
        int var_no = order.next();
        cout << "Next variable: #" << var_no << endl;
        Abstraction *other_abstraction = atomic_abstractions[var_no];

        // TODO: When using nonlinear merge strategies, make sure not
        // to normalize multiple parts of a composite. See issue68.
        if (shrink_strategy->reduce_labels_before_shrinking()) {
            abstraction->normalize(use_label_reduction);
            other_abstraction->normalize(false);
        }

        abstraction->compute_distances();
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

        abstraction->normalize(use_label_reduction);
        abstraction->statistics(use_expensive_statistics);

        // Don't label-reduce the atomic abstraction -- see issue68.
        other_abstraction->normalize(false);
        other_abstraction->statistics(use_expensive_statistics);

        Abstraction *new_abstraction = new CompositeAbstraction(
            is_unit_cost_problem(), get_cost_type(),
            abstraction, other_abstraction);

        abstraction->release_memory();
        other_abstraction->release_memory();

        abstraction = new_abstraction;
        abstraction->statistics(use_expensive_statistics);
    }

    abstraction->compute_distances();
    if (!abstraction->is_solvable())
        return abstraction;

    ShrinkStrategy *def_shrink = ShrinkFH::create_default(abstraction->size());
    def_shrink->shrink(*abstraction, abstraction->size(), true);
    abstraction->compute_distances();

    abstraction->statistics(use_expensive_statistics);
    abstraction->release_memory();
    return abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms_no_cond_effects();
    int peak_memory = 0;

    for (int i = 0; i < abstraction_count; i++) {
        cout << "Building abstraction #" << (i + 1) << "..." << endl;
        Abstraction *abstraction = build_abstraction(i == 0);
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        if (!abstractions.back()->is_solvable()) {
            cout << "Abstract problem is unsolvable!" << endl;
            if (i + 1 < abstraction_count)
                cout << "Skipping remaining abstractions." << endl;
            break;
        }
    }

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl << "initial h value: " << compute_heuristic(
        *g_initial_state) << endl;
    cout << "Estimated peak memory for abstraction: " << peak_memory << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const State &state) {
    int cost = 0;
    for (int i = 0; i < abstractions.size(); i++) {
        int abs_cost = abstractions[i]->get_cost(state);
        if (abs_cost == -1)
            return DEAD_END;
        cost = max(cost, abs_cost);
    }
    return cost;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    // TODO: better documentation what each parameter does
    parser.add_option<int>("count", 1, "nr of abstractions to build");
    vector<string> merge_strategies;
    //TODO: it's a bit dangerous that the merge strategies here
    // have to be specified exactly in the same order
    // as in the enum definition. Try to find a way around this,
    // or at least raise an error when the order is wrong.
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_RANDOM");
    merge_strategies.push_back("MERGE_LINEAR_GOAL_CG_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_RANDOM");
    merge_strategies.push_back("MERGE_DFP");
    merge_strategies.push_back("MERGE_LINEAR_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_REVERSE_LEVEL");
    parser.add_enum_option("merge_strategy", merge_strategies,
                           "MERGE_LINEAR_CG_GOAL_LEVEL",
                           "merge strategy");

    // TODO: Default shrink strategy should only be created
    // when it's actually used.
    ShrinkStrategy *def_shrink = ShrinkFH::create_default(50000);

    parser.add_option<ShrinkStrategy *>("shrink_strategy", def_shrink, "shrink strategy");
    // TODO: Rename option name to "use_label_reduction" to be
    //       consistent with the papers?
    parser.add_option<bool>("reduce_labels", true, "enable label reduction");
    parser.add_option<bool>("expensive_statistics", false, "show statistics on \"unique unlabeled edges\" (WARNING: "
                            "these are *very* slow -- check the warning in the output)");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (parser.dry_run()) {
        return 0;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<ScalarEvaluator> _plugin("merge_and_shrink", _parse);
