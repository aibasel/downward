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


static ScalarEvaluatorPlugin merge_and_shrink_heuristic_plugin(
    "mas", MergeAndShrinkHeuristic::create);


MergeAndShrinkHeuristic::MergeAndShrinkHeuristic(
    HeuristicOptions &options,
    int max_abstract_states_, int max_abstract_states_before_merge_,
    int abstraction_count_,
    MergeStrategy merge_strategy_, ShrinkStrategy shrink_strategy_,
    bool use_label_simplification_, bool use_expensive_statistics_)
    : Heuristic(options),
      max_abstract_states(max_abstract_states_),
      max_abstract_states_before_merge(max_abstract_states_before_merge_),
      abstraction_count(abstraction_count_),
      merge_strategy(merge_strategy_),
      shrink_strategy(shrink_strategy_),
      use_label_simplification(use_label_simplification_),
      use_expensive_statistics(use_expensive_statistics_) {
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

    /* TODO: The peak memory reported in the next line is wrong --
             this seems to be the maximum over the memory amounts
             required by the *last* abstraction of each iteration,
             rather than the peaks of each iteration.
             Abstraction::peak_memory doesn't seem to contain what
             we're interested in -- need to set it based on the child
             values for composite abstractions?

       TODO: Since this memory estimate is just an estimate, might be a
             good idea to complement it with a report on the peak memory
             usage of the process as provided in utilities.h.
    */
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

ScalarEvaluator *MergeAndShrinkHeuristic::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    int max_states = -1;
    int max_states_before_merge = -1;
    int abstraction_count = 1;
    int merge_strategy = MERGE_LINEAR_CG_GOAL_LEVEL;
    int shrink_strategy = SHRINK_HIGH_F_LOW_H;
    bool use_label_simplification = true;
    bool use_expensive_statistics = false;
    HeuristicOptions common_options;

    // "<name>()" or "<name>(<options>)"
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;

        // TODO: better documentation what each parameter does
        if (config[end] != ")") {
            NamedOptionParser option_parser;

            common_options.add_option_to_parser(option_parser);

            option_parser.add_int_option(
                "max_states",
                &max_states,
                "maximum abstraction size");
            option_parser.add_int_option(
                "max_states_before_merge",
                &max_states_before_merge,
                "maximum abstraction size for factors of synchronized product");
            option_parser.add_int_option(
                "count",
                &abstraction_count,
                "nr of abstractions to build");
            option_parser.add_int_option(
                "merge_strategy",
                &merge_strategy,
                "merge strategy");
            option_parser.add_int_option(
                "shrink_strategy",
                &shrink_strategy,
                "shrink strategy");
            option_parser.add_bool_option(
                "simplify_labels",
                &use_label_simplification,
                "enable label simplification");
            option_parser.add_bool_option(
                "expensive_statistics",
                &use_expensive_statistics,
                "show statistics on \"unique unlabeled edges\" (WARNING: "
                "these are *very* slow -- check the warning in the output)");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else { // "<name>"
        end = start;
    }

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
        if (max_states < 0 || max_states / n != n) // overflow
            max_states = numeric_limits<int>::max();
    }

    if (max_states_before_merge > max_states) {
        cerr << "warning: max_states_before_merge exceeds max_states, "
             << "correcting." << endl;
        max_states_before_merge = max_states;
    }

    if (max_states < 1) {
        cerr << "error: abstraction size must be at least 1"
             << endl;
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

    if (shrink_strategy < 0 || shrink_strategy >= MAX_SHRINK_STRATEGY) {
        cerr << "error: unknown shrink strategy: " << shrink_strategy << endl;
        exit(2);
    }

    if (dry_run) {
        return 0;
    } else {
        MergeAndShrinkHeuristic *result = new MergeAndShrinkHeuristic(
            common_options,
            max_states,
            max_states_before_merge,
            abstraction_count,
            static_cast<MergeStrategy>(merge_strategy),
            static_cast<ShrinkStrategy>(shrink_strategy),
            use_label_simplification,
            use_expensive_statistics);
        return result;
    }
}
