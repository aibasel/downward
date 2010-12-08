#include "mas_heuristic.h"

#include "abstraction.h"
#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "plugin.h"
#include "state.h"
#include "timer.h"
#include "variable_order_finder.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
using namespace std;


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(const Options &opts)
    : max_abstract_states(opts.get<int>("max_states")),
      max_abstract_states_before_merge(opts.get<int>("max_states_before_merge")),
      abstraction_count(opts.get<int>("count")),
      merge_strategy(MergeStrategy(opts.get<int>("merge_strategy"))),
      shrink_strategy(ShrinkStrategy(opts.get<int>("shrink_strategy"))),
      use_label_simplification_(opts.get<bool>("simplify_labels")),
      use_expensive_statistics(opts.get<bool>("expensive_statistics")) {
    assert(max_abstract_states_before_merge > 0);
    assert(max_abstract_states >= max_abstract_states_before_merge);
}

MergeAndShrinkHeuristic::~MergeAndShrinkHeuristic() {
}

void MergeAndShrinkHeuristic::dump_options() const {
    cout << "Abstraction size limit: "
         << max_abstract_states << endl
         << "Abstraction size limit right before merge: "
         << max_abstract_states_before_merge << endl
         << "Number of abstractions to maximize over: "
         << abstraction_count << endl
         << "Merge strategy: ";
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
    default:
        abort();
    }
    cout << endl
         << "Shrink strategy: ";
    switch (shrink_strategy) {
    case SHRINK_HIGH_F_LOW_H:
        cout << "high f/low h (main)";
        break;
    case SHRINK_LOW_F_LOW_H:
        cout << "low f/low h";
        break;
    case SHRINK_HIGH_F_HIGH_H:
        cout << "high f/high h";
        break;
    case SHRINK_RANDOM:
        cout << "random states";
        break;
    case SHRINK_DFP:
        cout << "Draeger/Finkbeiner/Podelski";
        break;
    default:
        abort();
    }
    cout << endl
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

void MergeAndShrinkHeuristic::verify_no_axioms_no_cond_effects() const {
    if (!g_axioms.empty()) {
        cerr << "Heuristic does not support axioms!" << endl
             << "Terminating." << endl;
        exit(1);
    }
    if (g_use_metric) {
        cerr << "Warning: M&S heuristic does not support action costs!" << endl;
        if (g_min_action_cost == 0) {
            cerr << "Alert: 0-cost actions exist. M&S Heuristic is not admissible" << endl;
        }
    }


    for (int i = 0; i < g_operators.size(); i++) {
        const vector<PrePost> &pre_post = g_operators[i].get_pre_post();
        for (int j = 0; j < pre_post.size(); j++) {
            const vector<Prevail> &cond = pre_post[j].cond;
            if (cond.empty())
                continue;
            // Accept conditions that are redundant, but nothing else.
            // In a better world, these would never be included in the
            // input in the first place.
            int var = pre_post[j].var;
            int pre = pre_post[j].pre;
            int post = pre_post[j].post;
            if (pre == -1 && cond.size() == 1 &&
                cond[0].var == var && cond[0].prev != post &&
                g_variable_domain[var] == 2)
                continue;

            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")"
                 << endl << "Terminating." << endl;
            exit(1);
        }
    }
}

pair<int, int> MergeAndShrinkHeuristic::compute_shrink_sizes(
    int size1, int size2) const {
    // Bound both sizes by max allowed size before merge.
    int newsize1 = min(size1, max_abstract_states_before_merge);
    int newsize2 = min(size2, max_abstract_states_before_merge);

    // Check if product would exceeds max allowed size.
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

    Abstraction *abstraction = atomic_abstractions[order.next()];
    abstraction->statistics(use_expensive_statistics);

    bool first_iteration = true;
    while (!order.done() && abstraction->is_solvable()) {
        int var_no = order.next();
        Abstraction *other_abstraction = atomic_abstractions[var_no];

        pair<int, int> new_sizes = compute_shrink_sizes(
            abstraction->size(), other_abstraction->size());
        int new_size = new_sizes.first;
        int other_new_size = new_sizes.second;

        if (other_new_size != other_abstraction->size()) {
            cout << "atomic abstraction too big; must shrink" << endl;
            other_abstraction->shrink(other_new_size, shrink_strategy);
        }

        if (new_size != abstraction->size()) {
            abstraction->shrink(new_size, shrink_strategy);
            abstraction->statistics(use_expensive_statistics);
        }
        Abstraction *new_abstraction = new CompositeAbstraction(
            abstraction, other_abstraction, use_label_simplification);
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
    cout << "Initializing merge-and-shrink heuristic..." << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms_no_cond_effects();

    int peak_memory = 0;
    for (int i = 0; i < abstraction_count; i++) {
        cout << "Building abstraction nr " << i << "..." << endl;
        Abstraction *abstraction = build_abstraction(i == 0);
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        if (!abstractions.back()->is_solvable())
            break;
    }

    cout << "Done initializing merge-and-shrink heuristic ["
         << timer << "]" << endl
         << "initial h value: " << compute_heuristic(*g_initial_state)
         << endl;
    cout << "Estimated peak memory: " << peak_memory << " bytes" << endl;
}

int MergeAndShrinkHeuristic::compute_heuristic(const State &state) {
    int cost = 0;
    for (int i = 0; i < abstractions.size(); i++) {
        int abs_cost = abstractions[i]->get_cost(state);
        if (abs_cost == -1)
            return DEAD_END;
        cost = max(cost, abs_cost);
    }
    if (cost == 0) {
        /* We don't want to report 0 for non-goal states because the
           search code doesn't like that. Note that we might report 0
           for non-goal states if we use tiny abstraction sizes (like
           1) or random shrinking. */
        // TODO: Change this once we support action costs!
        for (int i = 0; i < g_goal.size(); i++) {
            int var = g_goal[i].first, value = g_goal[i].second;
            if (state[var] != value) {
                cost = 1;
                break;
            }
        }
    }
    return cost;
}

ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<int>(
        "max_states", -1, "maximum abstraction size");
    parser.add_option<int>(
        "max_states_before_merge", -1,
        "maximum abstraction size for factors of synchronized product");
    parser.add_option<int>(
        "count", 1, "number of abstractions to build");

    vector<string> merge_choices;
    merge_choices.push_back("linear_cg_goal_level");
    merge_choices.push_back("linear_cg_goal_random");
    merge_choices.push_back("linear_goal_cg_level");
    merge_choices.push_back("linear_random");
    merge_choices.push_back("dfp");
    parser.add_enum_option(
        "merge_strategy", merge_choices, "linear_cg_goal_level",
        "merge strategy");

    vector<string> shrink_choices;
    shrink_choices.push_back("high_f_low_h");
    shrink_choices.push_back("low_f_low_h");
    shrink_choices.push_back("high_f_high_h");
    shrink_choices.push_back("random");
    shrink_choices.push_back("dfp");
    parser.add_enum_option(
        "shrink_strategy", shrink_choices, "high_f_low_h",
        "shrink strategy");

    parser.add_option<bool>(
        "simplify_labels", true, "enable label simplification");
    parser.add_option<bool>(
        "expensive_statistics", false,
        "show statistics on \"unique unlabeled edges\" (WARNING: "
        "these are *very* slow -- check the warning in the output)");

    Options opts = parser.get_configuration();

    // Handle default values for the size options.
    int max_states = opts.get<int>("max_states");
    int max_states_before_merge = opts.get<int>("max_states_before_merge");

    // If none of the two options was specified, use default limit.
    if (max_states == -1 && max_states_before_merge == -1)
        max_states = 50000;

    // If exactly one of the options is set now, set the other in
    // such a way that it imposes no further limits.
    if (max_states_before_merge == -1) {
        max_states_before_merge = max_states;
    } else if (max_states == -1) {
        int n = max_states_before_merge;
        max_states = n * n;
        if (max_states <= 0 || max_states / n != n) // overflow
            max_states = numeric_limits<int>::max();
    }

    // Write back updated values.
    opts.set<int>("max_states", max_states);
    opts.set<int>("max_states_before_merge", max_states_before_merge);

    // Normalize values: 1 <= max_states_before_merge <= max_states.
    if (max_states < 1 || max_states_before_merge < 1)
        parser.error("abstraction size must be at least 1");
    if (max_states_before_merge > max_states) {
        parser.warning("max_states_before_merge exceeds max_states, "
                       "correcting.");
        max_states_before_merge = max_states;
    }

    if (parser.dry_run)
        return 0;
    else
        return new MergeAndShrinkHeuristic(opts);
}

static ScalarEvaluatorPlugin _plugin("mas", _parse);
