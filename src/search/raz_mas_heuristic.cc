#include "raz_mas_heuristic.h"

#include "raz_abstraction.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "shrink_bisimulation.h"
#include "shrink_fh.h"
#include "state.h"
#include "timer.h"
#include "raz_variable_order_finder.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
using namespace std;


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : Heuristic(opts),
      max_abstract_states(opts.get<int>("max_states")),
      max_abstract_states_before_merge(opts.get<int>("max_states_before_merge")),
      abstraction_count(opts.get<int>("count")),
      merge_strategy(MergeStrategy(opts.get_enum("merge_strategy"))),
      shrink_strategy(opts.get<ShrinkStrategy *>("shrink_strategy")),
      use_label_simplification(opts.get<bool>("simplify_labels")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
    assert(max_abstract_states_before_merge > 0);
    assert(max_abstract_states >= max_abstract_states_before_merge);
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
}

void MergeAndShrinkHeuristic::dump_options() const {
    cout << "Abstraction size limit: " << max_abstract_states << endl
         << "Abstraction size limit right before merge: "
         << max_abstract_states_before_merge << endl
         << "Number of abstractions to maximize over: " << abstraction_count
         << endl << "Merge strategy: ";
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
        exit(2);
    case MERGE_LINEAR_LEVEL:
        cout << "linear by level";
        break;
    case MERGE_LINEAR_REVERSE_LEVEL:
        cout << "linear by reverse level";
        break;
    default:
        abort();
    }
    cout << endl
         << "Shrink strategy: " << shrink_strategy->description() << endl
         << "Label simplification: " 
         << (use_label_simplification ? "enabled" : "disabled") << endl
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

pair<int, int> MergeAndShrinkHeuristic::compute_shrink_sizes(int size1,
                                                             int size2) const {
    // Bound both sizes by max allowed size before merge.
    int newsize1 = min(size1, max_abstract_states_before_merge);
    int newsize2 = min(size2, max_abstract_states_before_merge);

    // Check if product would exceed max allowed size.
    // Use division instead of multiplication to avoid overflow.
    if (max_abstract_states / newsize1 < newsize2) {
        int balanced_size = int(sqrt(max_abstract_states));

        // Shrink size2 (which in the linear strategies is the size
        // for the atomic abstraction) down to balanced_size if larger.
        newsize2 = min(newsize2, balanced_size);

        // Use whatever is left for size1.
        newsize1 = min(newsize1, max_abstract_states / newsize2);
    }
    assert(newsize1 <= size1 && newsize2 <= size2);
    assert(newsize1 <= max_abstract_states_before_merge);
    assert(newsize2 <= max_abstract_states_before_merge);
    assert(newsize1 * newsize2 <= max_abstract_states);
    return make_pair(newsize1, newsize2);
}

Abstraction *MergeAndShrinkHeuristic::build_abstraction(bool is_first) {
    // TODO: We're leaking memory here in various ways. Fix this.
    //       Don't forget that build_atomic_abstractions also
    //       allocates memory.

    cout << "Building atomic abstractions..." << endl;
    vector<Abstraction *> atomic_abstractions;
    Abstraction::build_atomic_abstractions(atomic_abstractions);

    cout << "Merging abstractions..." << endl;

    VariableOrderFinder order(merge_strategy, is_first);

    int var_no = order.next();
    cout << "next variable: #" << var_no << endl;
    Abstraction *abstraction = atomic_abstractions[var_no];
    abstraction->statistics(use_expensive_statistics);

    bool first_iteration = true;
    while (!order.done() && abstraction->is_solvable()) {
        int var_no = order.next();
        cout << "next variable: #" << var_no << endl;
        Abstraction *other_abstraction = atomic_abstractions[var_no];

        pair<int, int> new_sizes = compute_shrink_sizes(abstraction->size(),
                                                        other_abstraction->size());
        int new_size = new_sizes.first;
        int other_new_size = new_sizes.second;


        if (other_new_size != other_abstraction->size()) {
            //				&& !is_bisimulation_strategy) {
            cout << "atomic abstraction too big; must shrink" << endl;
            if (shrink_strategy->has_memory_limit())
                shrink_strategy->shrink(*other_abstraction, other_new_size);
            else if (shrink_strategy->is_bisimulation()){
                ShrinkBisimulation nolimit(false, false);
                nolimit.shrink(*other_abstraction, 1000000000, false);
            }
        }
        //TODO - always shrink non-atomic abstraction in no memory limit strategies
        if (new_size != abstraction->size()
            || (!shrink_strategy->has_memory_limit()
                && shrink_strategy->is_bisimulation())) {
            shrink_strategy->shrink(*abstraction, new_size);
            abstraction->statistics(use_expensive_statistics);
        }

        bool
            normalize_after_composition =
            (shrink_strategy->is_bisimulation()
             || shrink_strategy->is_dfp())
            && use_label_simplification;
        Abstraction *new_abstraction = new CompositeAbstraction(abstraction,
                                                                other_abstraction, use_label_simplification,
                                                                normalize_after_composition);

        if (first_iteration)
            first_iteration = false;
        else
            abstraction->release_memory();
        abstraction = new_abstraction;
        abstraction->statistics(use_expensive_statistics);
    }
    return abstraction;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic...!!!" << endl;
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
    parser.add_option<int>("max_states", -1, "maximum abstraction size");
    parser.add_option<int>("max_states_before_merge", -1,
                           "maximum abstraction size for factors of synchronized product");
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
    
    //TODO: Default Shrink-Strategy should only be created
    // when it's actually used
    ShrinkStrategy *def_shrink = new ShrinkFH(HIGH, LOW);
    parser.add_option<ShrinkStrategy *>("shrink_strategy", def_shrink, "shrink strategy");
    parser.add_option<bool>("simplify_labels", true, "enable label simplification");
    parser.add_option<bool>("expensive_statistics", false, "show statistics on \"unique unlabeled edges\" (WARNING: "
                            "these are *very* slow -- check the warning in the output)");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    //read values from opts for processing.
    int max_states = opts.get<int>("max_states");
    int max_states_before_merge = opts.get<int>("max_states_before_merge");
    MergeStrategy merge_strategy = MergeStrategy(opts.get_enum("merge_strategy"));

    if (max_states == -1 && max_states_before_merge == -1) {
        // None of the two options specified: set default limit
        max_states = 50000;
    }

    // If exactly one of the max_states options has been set, set the other
    // so that it imposes no further limits.
    if (max_states_before_merge == -1) {
        max_states_before_merge = max_states;
    } else if (max_states == -1) {
        int n = max_states_before_merge;
        max_states = n * n;
        if (max_states < 0 || max_states / n != n)         // overflow
            max_states = numeric_limits<int>::max();
    }

    if (max_states_before_merge > max_states) {
        cerr << "warning: max_states_before_merge exceeds max_states, "
             << "correcting." << endl;
        max_states_before_merge = max_states;
    }

    if (max_states < 1) {
        cerr << "error: abstraction size must be at least 1" << endl;
        exit(2);
    }

    if (max_states_before_merge < 1) {
        cerr << "error: abstraction size before merge must be at least 1"
             << endl;
        exit(2);
    }

    if (merge_strategy < 0 || merge_strategy >= MAX_MERGE_STRATEGY) {
        cerr << "error: unknown merge strategy: " << merge_strategy << endl;
        exit(2);
    }

    //write values back:
    opts.set<int>("max_states", max_states);
    opts.set<int>("max_states_before_merge", max_states_before_merge);

    if (parser.dry_run()) {
        return 0;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(opts);
        return result;
    }
}

static Plugin<ScalarEvaluator> _plugin("mas", _parse);
