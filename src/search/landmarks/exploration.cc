#include "exploration.h"
#include "../global_operator.h"
#include "../global_state.h"
#include "../globals.h"
#include "../utilities.h"
#include "../utilities_hash.h"

#include <algorithm>
#include <cassert>
#include <limits>

using namespace std;

/* Integration Note: this class is the same as (rich man's) FF heuristic
   (taken from hector branch) except for the following:
   - Added-on functionality for excluding certain operators from the relaxed
     exploration (these operators are never applied, as necessary for landmark
     computation)
   - Exploration can be done either using h_max or h_add criterion (h_max is needed
     during landmark generation, h_add later for heuristic computations)
   - Unary operators are not simplified, because this may conflict with excluded
     operators. (For an example, consider that unary operator o1 is thrown out
     during simplify() because it is dominated by unary operator o2, but then o2
     is excluded during an exploration ==> the shared effect of o1 and o2 is wrongly
     never reached in the exploration.)
   - Added-on functionality to check for a set of propositions which one is reached
     first by the relaxed exploration, and extract preferred operators for this cheapest
     proposition (needed for planning to nearest landmark).
*/

// Construction and destruction
Exploration::Exploration(const Options &opts)
    : Heuristic(opts),
      did_write_overflow_warning(false) {
    cout << "Initializing Exploration..." << endl;

    // Build propositions.
    for (size_t var = 0; var < g_variable_domain.size(); ++var) {
        propositions.push_back(vector<ExProposition>(g_variable_domain[var]));
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            propositions[var][val].var = var;
            propositions[var][val].val = val;
        }
    }

    // Build goal propositions.
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int var = g_goal[i].first, val = g_goal[i].second;
        propositions[var][val].is_goal_condition = true;
        propositions[var][val].is_termination_condition = true;
        goal_propositions.push_back(&propositions[var][val]);
        termination_propositions.push_back(&propositions[var][val]);
    }

    // Build unary operators for operators and axioms.
    for (size_t i = 0; i < g_operators.size(); ++i)
        build_unary_operators(g_operators[i]);
    for (size_t i = 0; i < g_axioms.size(); ++i)
        build_unary_operators(g_axioms[i]);

    // Cross-reference unary operators.
    for (size_t i = 0; i < unary_operators.size(); ++i) {
        ExUnaryOperator *op = &unary_operators[i];
        for (size_t j = 0; j < op->precondition.size(); ++j)
            op->precondition[j]->precondition_of.push_back(op);
    }
    // Set flag that before heuristic values can be used, computation
    // (relaxed exploration) needs to be done
    heuristic_recomputation_needed = true;
}

Exploration::~Exploration() {
}

void Exploration::increase_cost(int &cost, int amount) {
    assert(cost >= 0);
    assert(amount >= 0);
    cost += amount;
    if (cost > MAX_COST_VALUE) {
        write_overflow_warning();
        cost = MAX_COST_VALUE;
    }
}

void Exploration::write_overflow_warning() {
    if (!did_write_overflow_warning) {
        // TODO: Should have a planner-wide warning mechanism to handle
        // things like this.
        cout << "WARNING: overflow on LAMA/FF synergy h^add! Costs clamped to "
             << MAX_COST_VALUE << endl;
        cout << "WARNING: overflow on LAMA/FF synergy h^add! Costs clamped to "
             << MAX_COST_VALUE << endl;
        did_write_overflow_warning = true;
    }
}

void Exploration::set_additional_goals(const vector<pair<int, int> > &add_goals) {
    //Clear previous additional goals.
    for (size_t i = 0; i < termination_propositions.size(); ++i) {
        int var = termination_propositions[i]->var, val = termination_propositions[i]->val;
        propositions[var][val].is_termination_condition = false;
    }
    termination_propositions.clear();
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int var = g_goal[i].first, val = g_goal[i].second;
        propositions[var][val].is_termination_condition = true;
        termination_propositions.push_back(&propositions[var][val]);
    }
    // Build new additional goal propositions.
    for (size_t i = 0; i < add_goals.size(); ++i) {
        int var = add_goals[i].first, val = add_goals[i].second;
        if (!propositions[var][val].is_goal_condition) {
            propositions[var][val].is_termination_condition = true;
            termination_propositions.push_back(&propositions[var][val]);
        }
    }
    heuristic_recomputation_needed = true;
}

void Exploration::build_unary_operators(const GlobalOperator &op) {
    // Note: changed from the original to allow sorting of operator conditions
    int base_cost = get_adjusted_cost(op);
    const vector<GlobalCondition> &preconditions = op.get_preconditions();
    const vector<GlobalEffect> &effects = op.get_effects();
    vector<ExProposition *> precondition;
    vector<pair<int, int> > precondition_var_vals1;

    for (size_t i = 0; i < preconditions.size(); ++i) {
        assert(in_bounds(preconditions[i].var, g_variable_domain));
        assert(preconditions[i].val >= 0 && preconditions[i].val < g_variable_domain[preconditions[i].var]);
        precondition_var_vals1.push_back(make_pair(preconditions[i].var, preconditions[i].val));
    }
    for (size_t i = 0; i < effects.size(); ++i) {
        vector<pair<int, int> > precondition_var_vals2(precondition_var_vals1);
        assert(in_bounds(effects[i].var, g_variable_domain));
        assert(effects[i].val >= 0 && effects[i].val < g_variable_domain[effects[i].var]);
        ExProposition *effect = &propositions[effects[i].var][effects[i].val];
        const vector<GlobalCondition> &eff_conds = effects[i].conditions;
        for (size_t j = 0; j < eff_conds.size(); ++j) {
            assert(in_bounds(eff_conds[j].var, g_variable_domain));
            assert(eff_conds[j].val >= 0 && eff_conds[j].val < g_variable_domain[eff_conds[j].var]);
            precondition_var_vals2.push_back(make_pair(eff_conds[j].var, eff_conds[j].val));
        }

        sort(precondition_var_vals2.begin(), precondition_var_vals2.end());

        for (size_t j = 0; j < precondition_var_vals2.size(); ++j)
            precondition.push_back(&propositions[precondition_var_vals2[j].first]
                                   [precondition_var_vals2[j].second]);

        unary_operators.push_back(ExUnaryOperator(precondition, effect, &op, base_cost));
        precondition.clear();
        precondition_var_vals2.clear();
    }
}

// heuristic computation
void Exploration::setup_exploration_queue(const GlobalState &state,
                                          const vector<pair<int, int> > &excluded_props,
                                          const unordered_set<const GlobalOperator *> &excluded_ops,
                                          bool use_h_max = false) {
    prop_queue.clear();

    for (size_t var = 0; var < propositions.size(); ++var) {
        for (size_t value = 0; value < propositions[var].size(); ++value) {
            ExProposition &prop = propositions[var][value];
            prop.h_add_cost = -1;
            prop.h_max_cost = -1;
            prop.depth = -1;
            prop.marked = false;
        }
    }
    if (excluded_props.size() > 0) {
        for (size_t i = 0; i < excluded_props.size(); ++i) {
            ExProposition &prop = propositions[excluded_props[i].first][excluded_props[i].second];
            prop.h_add_cost = -2;
        }
    }

    // Deal with current state.
    for (size_t var = 0; var < propositions.size(); ++var) {
        ExProposition *init_prop = &propositions[var][state[var]];
        enqueue_if_necessary(init_prop, 0, 0, 0, use_h_max);
    }

    // Initialize operator data, deal with precondition-free operators/axioms.
    for (size_t i = 0; i < unary_operators.size(); ++i) {
        ExUnaryOperator &op = unary_operators[i];
        op.unsatisfied_preconditions = op.precondition.size();
        if (excluded_ops.size() > 0 && (op.effect->h_add_cost == -2 ||
                                        excluded_ops.find(op.op) != excluded_ops.end())) {
            op.h_add_cost = -2; // operator will not be applied during relaxed exploration
            continue;
        }
        op.h_add_cost = op.base_cost; // will be increased by precondition costs
        op.h_max_cost = op.base_cost;
        op.depth = -1;

        if (op.unsatisfied_preconditions == 0) {
            op.depth = 0;
            int depth = op.op->is_axiom() ? 0 : 1;
            enqueue_if_necessary(op.effect, op.base_cost, depth, &op, use_h_max);
        }
    }
}

void Exploration::relaxed_exploration(bool use_h_max = false, bool level_out = false) {
    int unsolved_goals = termination_propositions.size();
    while (!prop_queue.empty()) {
        pair<int, ExProposition *> top_pair = prop_queue.pop();
        int distance = top_pair.first;
        ExProposition *prop = top_pair.second;

        int prop_cost;
        if (use_h_max)
            prop_cost = prop->h_max_cost;
        else
            prop_cost = prop->h_add_cost;
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        if (!level_out && prop->is_termination_condition && --unsolved_goals == 0)
            return;
        const vector<ExUnaryOperator *> &triggered_operators = prop->precondition_of;
        for (size_t i = 0; i < triggered_operators.size(); ++i) {
            ExUnaryOperator *unary_op = triggered_operators[i];
            if (unary_op->h_add_cost == -2) // operator is not applied
                continue;
            --unary_op->unsatisfied_preconditions;
            increase_cost(unary_op->h_add_cost, prop_cost);
            unary_op->h_max_cost = max(prop_cost + unary_op->base_cost,
                                       unary_op->h_max_cost);
            unary_op->depth = max(unary_op->depth, prop->depth);
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                int depth = unary_op->op->is_axiom() ? unary_op->depth : unary_op->depth + 1;
                if (use_h_max)
                    enqueue_if_necessary(unary_op->effect, unary_op->h_max_cost,
                                         depth, unary_op, use_h_max);
                else
                    enqueue_if_necessary(unary_op->effect, unary_op->h_add_cost,
                                         depth, unary_op, use_h_max);
            }
        }
    }
}

void Exploration::enqueue_if_necessary(ExProposition *prop, int cost, int depth,
                                       ExUnaryOperator *op,
                                       bool use_h_max) {
    assert(cost >= 0);
    if (use_h_max && (prop->h_max_cost == -1 || prop->h_max_cost > cost)) {
        prop->h_max_cost = cost;
        prop->depth = depth;
        prop->reached_by = op;
        prop_queue.push(cost, prop);
    } else if (!use_h_max && (prop->h_add_cost == -1 || prop->h_add_cost > cost)) {
        prop->h_add_cost = cost;
        prop->depth = depth;
        prop->reached_by = op;
        prop_queue.push(cost, prop);
    }
    if (use_h_max)
        assert(prop->h_max_cost != -1 &&
               prop->h_max_cost <= cost);
    else
        assert(prop->h_add_cost != -1 &&
               prop->h_add_cost <= cost);
}


int Exploration::compute_hsp_add_heuristic() {
    int total_cost = 0;
    for (size_t i = 0; i < goal_propositions.size(); ++i) {
        int prop_cost = goal_propositions[i]->h_add_cost;
        if (prop_cost == -1)
            return DEAD_END;
        increase_cost(total_cost, prop_cost);
    }
    return total_cost;
}


int Exploration::compute_ff_heuristic(const GlobalState &state) {
    int h_add_heuristic = compute_hsp_add_heuristic();
    if (h_add_heuristic == DEAD_END) {
        return DEAD_END;
    } else {
        relaxed_plan.clear();
        // Collecting the relaxed plan also marks helpful actions as preferred.
        for (size_t i = 0; i < goal_propositions.size(); ++i)
            collect_relaxed_plan(goal_propositions[i], relaxed_plan, state);
        int cost = 0;
        RelaxedPlan::iterator it = relaxed_plan.begin();
        for (; it != relaxed_plan.end(); ++it)
            cost += get_adjusted_cost(**it);
        return cost;
    }
}

void Exploration::collect_relaxed_plan(ExProposition *goal,
                                       RelaxedPlan &relaxed_plan, const GlobalState &state) {
    if (!goal->marked) { // Only consider each subgoal once.
        goal->marked = true;
        ExUnaryOperator *unary_op = goal->reached_by;
        if (unary_op) { // We have not yet chained back to a start node.
            for (size_t i = 0; i < unary_op->precondition.size(); ++i)
                collect_relaxed_plan(unary_op->precondition[i], relaxed_plan, state);
            const GlobalOperator *op = unary_op->op;
            bool added_to_relaxed_plan = false;
            //if(!op->is_axiom()) // Using axioms in the relaxed plan actually
            //improves performance in many domains... We need to look into this.
            added_to_relaxed_plan = relaxed_plan.insert(op).second;

            assert(unary_op->depth != -1);
            if (added_to_relaxed_plan
                && unary_op->h_add_cost == unary_op->base_cost
                && unary_op->depth == 0
                && !op->is_axiom()) {
                set_preferred(op);
                assert(op->is_applicable(state));
            }
        }
    }
}

void Exploration::compute_reachability_with_excludes(vector<vector<int> > &lvl_var,
                                                     vector<unordered_map<pair<int, int>, int> > &lvl_op,
                                                     bool level_out,
                                                     const vector<pair<int, int> > &excluded_props,
                                                     const unordered_set<const GlobalOperator *> &excluded_ops,
                                                     bool compute_lvl_ops) {
    // Perform exploration using h_max-values
    setup_exploration_queue(g_initial_state(), excluded_props, excluded_ops, true);
    relaxed_exploration(true, level_out);

    // Copy reachability information into lvl_var and lvl_op
    for (size_t var = 0; var < propositions.size(); ++var) {
        for (size_t value = 0; value < propositions[var].size(); ++value) {
            ExProposition &prop = propositions[var][value];
            if (prop.h_max_cost >= 0)
                lvl_var[var][value] = prop.h_max_cost;
        }
    }
    if (compute_lvl_ops) {
        unordered_map< const GlobalOperator *, int> operator_index;
        for (size_t i = 0; i < g_operators.size(); ++i) {
            operator_index.insert(make_pair(&g_operators[i], i));
        }
        int offset = g_operators.size();
        for (size_t i = 0; i < g_axioms.size(); ++i) {
            operator_index.insert(make_pair(&g_axioms[i], i + offset));
        }
        for (size_t i = 0; i < unary_operators.size(); ++i) {
            ExUnaryOperator &op = unary_operators[i];
            // H_max_cost of operator might be wrongly 0 or 1, if the operator
            // did not get applied during relaxed exploration. Look through
            // preconditions and adjust.
            for (size_t i = 0; i < op.precondition.size(); ++i) {
                ExProposition *prop = op.precondition[i];
                if (prop->h_max_cost == -1) {
                    // Operator cannot be applied due to unreached precondition
                    op.h_max_cost = numeric_limits<int>::max();
                    break;
                } else if (op.h_max_cost < prop->h_max_cost + op.base_cost)
                    op.h_max_cost = prop->h_max_cost + op.base_cost;
            }
            if (op.h_max_cost == numeric_limits<int>::max())
                break;
            int op_index = operator_index[op.op];
            // We subtract 1 to keep semantics for landmark code:
            // if op can achieve prop at time step i+1,
            // its index (for prop) is i, where the initial state is time step 0.
            pair<int, int> effect = make_pair(op.effect->var, op.effect->val);
            assert(lvl_op[op_index].find(effect) != lvl_op[op_index].end());
            int new_lvl = op.h_max_cost - 1;
            // If we have found a cheaper achieving operator, adjust h_max cost of proposition.
            if (lvl_op[op_index].find(effect)->second > new_lvl)
                lvl_op[op_index].find(effect)->second = new_lvl;
        }
    }
    heuristic_recomputation_needed = true;
}

void Exploration::prepare_heuristic_computation(const GlobalState &state, bool h_max = false) {
    setup_exploration_queue(state, h_max);
    relaxed_exploration(h_max);
    heuristic_recomputation_needed = false;
}

int Exploration::compute_heuristic(const GlobalState &state) {
    if (heuristic_recomputation_needed) {
        prepare_heuristic_computation(state);
    }
    return compute_ff_heuristic(state);
}


void Exploration::collect_ha(ExProposition *goal,
                             RelaxedPlan &relaxed_plan, const GlobalState &state) {
    // This is the same as collect_relaxed_plan, except that preferred operators
    // are saved in exported_ops rather than preferred_operators

    ExUnaryOperator *unary_op = goal->reached_by;
    if (unary_op) { // We have not yet chained back to a start node.
        for (size_t i = 0; i < unary_op->precondition.size(); ++i)
            collect_ha(unary_op->precondition[i], relaxed_plan, state);
        const GlobalOperator *op = unary_op->op;
        bool added_to_relaxed_plan = false;
        if (!op->is_axiom())
            added_to_relaxed_plan = relaxed_plan.insert(op).second;
        if (added_to_relaxed_plan
            && unary_op->h_add_cost == unary_op->base_cost
            && unary_op->depth == 0
            && !op->is_axiom()) {
            exported_ops.push_back(op); // This is a helpful action.
            assert(op->is_applicable(state));
        }
    }
}

// TODO: this should be in landmark class
bool is_landmark(vector<pair<int, int> > &landmarks, int var, int val) {
    // TODO: change landmarks to set or unordered_set
    for (size_t i = 0; i < landmarks.size(); ++i)
        if (landmarks[i].first == var && landmarks[i].second == val)
            return true;
    return false;
}

bool Exploration::plan_for_disj(vector<pair<int, int> > &landmarks,
                                const GlobalState &state) {
    relaxed_plan.clear();
    // generate plan to reach part of disj. goal OR if no landmarks given, plan to real goal
    if (!landmarks.empty()) {
        // search for quickest achievable landmark leaves
        if (heuristic_recomputation_needed) {
            prepare_heuristic_computation(state);
        }
        int min_cost = numeric_limits<int>::max();
        ExProposition *target = NULL;
        for (size_t i = 0; i < termination_propositions.size(); ++i) {
            const int prop_cost = termination_propositions[i]->h_add_cost;
            if (prop_cost == -1 && is_landmark(landmarks, termination_propositions[i]->var,
                                               termination_propositions[i]->val)) {
                return false; // dead end
            }
            if (prop_cost < min_cost && is_landmark(landmarks, termination_propositions[i]->var,
                                                    termination_propositions[i]->val)) {
                target = termination_propositions[i];
                min_cost = prop_cost;
            }
        }
        assert(target != NULL);
        assert(exported_ops.empty());
        collect_ha(target, relaxed_plan, state);
    } else {
        // search for original goals of the task
        if (heuristic_recomputation_needed) {
            prepare_heuristic_computation(state);
        }
        for (size_t i = 0; i < goal_propositions.size(); ++i) {
            if (goal_propositions[i]->h_add_cost == -1)
                return false;  // dead end
            collect_ha(goal_propositions[i], relaxed_plan, state);
        }
    }
    return true;
}
