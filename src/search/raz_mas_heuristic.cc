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
      use_expensive_statistics(opts.get<bool>("expensive_statistics")),
      merge_mixing_parameter(opts.get<double>("merge_mixing_parameter")) {
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
    case MERGE_LEVEL_THEN_INVERSE:
        cout
        << "linear mixing level and inverse level, first by level then inverse, mixing parameter is: "
        << merge_mixing_parameter;
        break;
    case MERGE_INVERSE_THEN_LEVEL:
        cout
        << "linear mixing level and inverse level, first inverse then by level, mixing parameter is: "
        << merge_mixing_parameter;
        break;
    default:
        abort();
    }
    cout << endl << "Shrink strategy: " << shrink_strategy->description();
    cout << endl << "Label simplification: "
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
        cerr << "Heuristic does not support axioms!" << endl << "Terminating."
             << endl;
        exit(1);
    }
    if (g_use_metric) {
        cerr << "Warning: M&S heuristic does not support action costs!" << endl;
        if (g_min_action_cost == 0) {
            cerr
            << "Alert: 0-cost actions exist. M&S Heuristic is not admissible"
            << endl;
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
            if (pre == -1 && cond.size() == 1 && cond[0].var == var
                && cond[0].prev != post && g_variable_domain[var] == 2)
                continue;

            cerr << "Heuristic does not support conditional effects "
                 << "(operator " << g_operators[i].get_name() << ")" << endl
                 << "Terminating." << endl;
            exit(1);
        }
    }
}

pair<int, int> MergeAndShrinkHeuristic::compute_shrink_sizes(int size1,
                                                             int size2) const {
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


    //	cout << "ALL OPERATORS: " << endl;
    //	for (int i = 0; i < g_operators.size(); i++)
    //		cout << i << ": " << g_operators[i].get_name() << endl;

    cout << "Building atomic abstractions..." << endl;
    vector<Abstraction *> atomic_abstractions;
    Abstraction::build_atomic_abstractions(atomic_abstractions);

    cout << "Merging abstractions..." << endl;

    VariableOrderFinder order(merge_strategy, merge_mixing_parameter, is_first);

    Abstraction *abstraction = atomic_abstractions[order.next()];
    abstraction->statistics(use_expensive_statistics);

    bool first_iteration = true;
    while (!order.done() && abstraction->is_solvable()) {
        int var_no = order.next();
        Abstraction *other_abstraction = atomic_abstractions[var_no];

        pair<int, int> new_sizes = compute_shrink_sizes(abstraction->size(),
                                                        other_abstraction->size());
        int new_size = new_sizes.first;
        int other_new_size = new_sizes.second;


        //TODO - never shrink atomic abstraction in no memory limit strategies
        if (other_new_size != other_abstraction->size()) {
            //				&& !is_bisimulation_strategy) {
            cout << "atomic abstraction too big; must shrink" << endl;
            if (!shrink_strategy->has_memory_limit() 
                || !shrink_strategy->is_bisimulation())
                shrink_strategy->shrink(*other_abstraction, other_new_size);
            else {
                ShrinkBisimulation nolimit(false, false);
                nolimit.shrink(*other_abstraction, 1000000000, false);
            }
        }
        //TODO - always shrink non-atomic abstraction in no memory limit strategies
        if (new_size != abstraction->size()
            || !shrink_strategy->has_memory_limit() 
            || !shrink_strategy->is_bisimulation()) {
            shrink_strategy->shrink(*abstraction, new_size);
            abstraction->statistics(use_expensive_statistics);
        }
        //TODO - use this for finding reducible labels!!!
        //		int label_reduce_strategy = 3;
        //
        //		vector<vector<const Operator *> > operator_groups_to_reduce;
        //		get_ops_to_reduce(abstraction, other_abstraction,
        //				operator_groups_to_reduce, label_reduce_strategy);

        //		cout << "Ops to reduce: " << endl;
        //		for (int group = 0; group < operator_groups_to_reduce.size(); group++) {
        //			cout << "Group no. " << group << endl;
        //			for (int op = 0; op < operator_groups_to_reduce[group].size(); ++op) {
        //				cout << operator_groups_to_reduce[group][op]->get_name()
        //						<< endl;
        //			}
        //		}
        //		cout << endl;

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
//NOT CURRENTLY USED
void MergeAndShrinkHeuristic::get_ops_to_reduce(Abstraction *abs1,
                                                Abstraction *abs2, vector<vector<const Operator *> > &result,
                                                int reduce_strategy) {
    //Strategy 1:
    //get the largest group of operators effecting the same variable
    int size_of_biggest_group = abs1->size() + abs2->size();

    size_of_biggest_group = 0;
    int num_of_groups = 0;
    vector<int> biggest_group_indices;     //groups are separated by -1 value;
    biggest_group_indices.reserve(g_operators.size() * 2);

    if (reduce_strategy == 1) {
        //Strategy 1:
        //get the largest group of operators effecting the same variable
        vector<int> current_group_indices;
        current_group_indices.reserve(g_operators.size() * 2);
        for (int var = 0; var < g_variable_domain.size(); var++) {
            current_group_indices.clear();
            for (int op = 0; op < g_operators.size(); op++) {
                const vector<PrePost> pre_post = g_operators[op].get_pre_post();
                for (int cond = 0; cond < pre_post.size(); cond++) {
                    if (pre_post[cond].var == var) {
                        current_group_indices.push_back(op);
                        break;
                    }
                }
            }
            if (current_group_indices.size() > size_of_biggest_group) {
                size_of_biggest_group = current_group_indices.size();
                biggest_group_indices.clear();
                biggest_group_indices.swap(current_group_indices);
                num_of_groups = 1;
            }
            if (current_group_indices.size() == size_of_biggest_group) {
                num_of_groups++;
                biggest_group_indices.push_back(-1);
                for (int i = 0; i < current_group_indices.size(); i++)
                    biggest_group_indices.push_back(current_group_indices[i]);
            }
        }
    } else if (reduce_strategy == 2) {
        //Strategy 2 (testing purposes) all labels
        num_of_groups = 1;
        for (int i = 0; i < g_operators.size(); i++)
            biggest_group_indices.push_back(i);
    } else if (reduce_strategy == 3) {
        //Strategy 3 (testing purposes) no labels
        num_of_groups = 0;
        biggest_group_indices.clear();
        result.clear();
    } else if (reduce_strategy == 4) {
        //Strategy 4 - similar operators (reduce all ops that are with distance
        // of less than delta from a seed operator)
        int delta = 2;         //this is what we iterate on
        vector<bool> already_used(g_operators.size(), false);
        for (int i = 0; i < g_operators.size(); i++) {
            if (!already_used[i]) {
                num_of_groups++;
                if (i > 0)
                    biggest_group_indices.push_back(-1);

                Operator *seed_op = &g_operators[i];
                biggest_group_indices.push_back(i);
                already_used[i] = true;
                for (int j = i + 1; j < g_operators.size(); j++) {
                    if (!already_used[j] && seed_op->get_distance_from_op(
                            &g_operators[j]) <= delta) {
                        biggest_group_indices.push_back(j);
                        already_used[j] = true;
                    }
                }
            }
        }
    }

    if (num_of_groups == 0 || num_of_groups == g_operators.size()) {
        //		cout<<"EMPTY OR ALL SINGLETON GROUPS"<<endl;
        return;
    }

    //Building the vector<vector<Operator *> > result
    result.reserve(num_of_groups);
    int current_group = 0;
    result.push_back(vector<const Operator *> (g_operators.size(), 0));
    result[current_group].clear();
    cout << "GROUPS ARE:" << endl;
    for (int i = 0; i < biggest_group_indices.size(); i++) {
        cout << biggest_group_indices[i] << ", ";
        if (biggest_group_indices[i] == -1) {         //new group
            current_group++;
            result.push_back(vector<const Operator *> (g_operators.size(), 0));
            result[current_group].clear();
        } else {
            result[current_group].push_back(
                &g_operators[biggest_group_indices[i]]);
        }
    }
}
//TODO NOT CURRENTLY USED - (Raz) Must be checked and revised! Not counting properly.
int MergeAndShrinkHeuristic::approximate_added_transitions(
    const vector<int> &transitions_to_reduce,
    const vector<int> &relevant_atomic_abs, const Abstraction *abstraction,
    const vector<Abstraction *> &atomic_abstractions) const {
    int sum_no_red = 0;
    for (int op_index = 0; op_index < transitions_to_reduce.size(); op_index++) {
        vector<int> op_vector(1, transitions_to_reduce[op_index]);
        int num_transition_main_abs =
            abstraction->unique_unlabeled_transitions(op_vector);
        //		cout << "pi_for_op of main abs: " << pi_for_op << endl;
        int pi_for_op = num_transition_main_abs > 0 ? num_transition_main_abs
                        : abstraction->size();
        for (int atomic_abs_index = 0; atomic_abs_index
             < relevant_atomic_abs.size(); atomic_abs_index++) {
            int atomic_var = relevant_atomic_abs[atomic_abs_index];
            if (!abstraction->is_in_varset(atomic_var)) {
                int
                    num_of_unique_unlabeled_transitions =
                    atomic_abstractions[atomic_var]->unique_unlabeled_transitions(
                        op_vector);
                if (num_of_unique_unlabeled_transitions == 0)
                    pi_for_op *= atomic_abstractions[atomic_var]->size();
                else
                    pi_for_op *= num_of_unique_unlabeled_transitions;
            }
        }
        sum_no_red += pi_for_op;
    }
    int sum_red = abstraction->unique_unlabeled_transitions(
        transitions_to_reduce);
    if (sum_red == 0)
        sum_red = abstraction->size();
    for (int atomic_abs_index = 0; atomic_abs_index
         < relevant_atomic_abs.size(); atomic_abs_index++) {
        int atomic_var = relevant_atomic_abs[atomic_abs_index];
        if (!abstraction->is_in_varset(atomic_var)) {
            int
                num_of_unique_unlabeled_transitions =
                atomic_abstractions[atomic_var]->unique_unlabeled_transitions(
                    transitions_to_reduce);
            if (num_of_unique_unlabeled_transitions == 0)
                sum_red *= atomic_abstractions[atomic_var]->size();
            else
                sum_red *= atomic_abstractions[atomic_var]->size();
        }
    }

    cout << "sum red " << sum_red << ", sum_no_red " << sum_no_red << endl;
    return sum_red - sum_no_red;
}

void MergeAndShrinkHeuristic::initialize() {
    Timer timer;
    cout << "Initializing merge-and-shrink heuristic...!!!" << endl;
    dump_options();
    warn_on_unusual_options();

    verify_no_axioms_no_cond_effects();
    //TODO - this is raz's experiment on two configurations:
    //conf 1 is what is given by the user in the cmd line.
    //conf 2 is the following: 5-12-1-true-none
    int peak_memory = 0;

    if (abstraction_count == -2) {
        cout << "Building abstraction nr 1..." << endl;
        Abstraction *abstraction = build_abstraction(true);
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        //if (!abstractions.back()->is_solvable())
        //	break;

        cout << "Building abstraction nr 2..." << endl;
        merge_strategy = MERGE_LINEAR_LEVEL;
        max_abstract_states = 1;
        max_abstract_states_before_merge = 1;
        abstraction = build_abstraction(true);
//        shrink_strategy = SHRINK_GREEDY_BISIMULATION_NO_MEMORY_LIMIT;
        abstraction->set_shrink_strategy(new ShrinkBisimulation(true, false));
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        //if (!abstractions.back()->is_solvable())
        //	break;
    }

    for (int i = 0; i < abstraction_count; i++) {
        cout << "Building abstraction nr " << i << "..." << endl;
        Abstraction *abstraction = build_abstraction(i == 0);
        peak_memory = max(peak_memory, abstraction->get_peak_memory_estimate());
        abstractions.push_back(abstraction);
        if (!abstractions.back()->is_solvable())
            break;
    }

    cout << "Done initializing merge-and-shrink heuristic [" << timer << "]"
         << endl << "initial h value: " << compute_heuristic(
        *g_initial_state) << endl;
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
                cost = g_min_action_cost;
                break;
            }
        }
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
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_CG_GOAL_RANDOM");
    merge_strategies.push_back("MERGE_LINEAR_GOAL_CG_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_RANDOM");
    merge_strategies.push_back("MERGE_DFP");
    merge_strategies.push_back("MERGE_LINEAR_LEVEL");
    merge_strategies.push_back("MERGE_LINEAR_REVERSE_LEVEL");
    merge_strategies.push_back("MERGE_LEVEL_THEN_INVERSE");
    merge_strategies.push_back("MERGE_INVERSE_THEN_LEVEL");
    parser.add_enum_option("merge_strategy", merge_strategies,
                           "MERGE_LINEAR_CG_GOAL_LEVEL",
                           "merge strategy");
    ShrinkStrategy *def_shrink = new ShrinkFH(HIGH, LOW);
    parser.add_option<ShrinkStrategy *>("shrink_strategy", def_shrink, "shrink strategy");
    parser.add_option<bool>("simplify_labels", true, "enable label simplification");
    parser.add_option<bool>("expensive_statistics", false, "show statistics on \"unique unlabeled edges\" (WARNING: "
                            "these are *very* slow -- check the warning in the output)");
    parser.add_option<double>("merge_mixing_parameter", -1.0, "merge mixing parameter");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    //read values from opts for processing.
    int max_states = opts.get<int>("max_states");
    int max_states_before_merge = opts.get<int>("max_states_before_merge");
    MergeStrategy merge_strategy = MergeStrategy(opts.get_enum("merge_strategy"));
    double merge_mixing_parameter = opts.get<double>("merge_mixing_parameter");


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

    if ((merge_strategy == MERGE_LEVEL_THEN_INVERSE || merge_strategy
         == MERGE_INVERSE_THEN_LEVEL) && (merge_mixing_parameter < 0.0
                                          || merge_mixing_parameter > 1.0)) {
        cerr << "error: mixed merging strategy with invalid mixing parameter: "
             << merge_mixing_parameter << endl;
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
