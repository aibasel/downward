#include "stubborn_sets_ec.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>

using namespace std;

namespace stubborn_sets_ec {
// DTGs are stored as one adjacency list per value.
using StubbornDTG = vector<vector<int>>;

// TODO: needs a central place (see comment for simple stubborn sets)
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && (uint)op_index < g_operators.size());
    return op_index;
}

static inline bool is_v_applicable(int var,
                                   int op_no,
                                   const GlobalState &state,
                                   std::vector<std::vector<int>> &preconditions) {
    int precondition_on_var = preconditions[op_no][var];
    return precondition_on_var == -1 || precondition_on_var == state[var];
}

// Copied from SimpleStubbornSets
static inline Fact find_unsatisfied_goal(const GlobalState &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        int goal_value = g_goal[i].second;
        if (state[goal_var] != goal_value)
            return Fact(goal_var, goal_value);
    }
    return Fact(-1, -1);
}

vector<StubbornDTG> build_dtgs() {
    /*
      NOTE: Code lifted and adapted from M&S atomic abstraction code.
      We need a more general mechanism for creating data structures of
      this kind.
     */

    /*
      NOTE: for stubborn sets ec, the DTG for v *does* include
      self-loops from d to d if there is an operator that sets the
      value of v to d and has no precondition on v. This is different
      from the usual DTG definition.
     */

    // Create the empty DTG nodes.
    vector<StubbornDTG> dtgs;
    int num_variables = g_variable_domain.size();
    for (int var_no = 0; var_no < num_variables; ++var_no) {
        dtgs.emplace_back(g_variable_domain[var_no]);
    }

    // Add DTG arcs.
    for (const GlobalOperator &op : g_operators) {
        for (const GlobalEffect &effect : op.get_effects()) {
            int eff_var = effect.var;
            int eff_val = effect.val;
            int pre_val = -1;

            for (const GlobalCondition &precondition : op.get_preconditions()) {
                if (precondition.var == eff_var) {
                    pre_val = precondition.val;
                    break;
                }
            }

            StubbornDTG &dtg = dtgs[eff_var];
            if (pre_val == -1) {
                for (int value = 0; value < g_variable_domain[eff_var]; ++value) {
                    dtg[value].push_back(eff_val);
                }
            } else {
                dtg[pre_val].push_back(eff_val);
            }
        }
    }
    return dtgs;
}

void recurse_forwards(const StubbornDTG &dtg,
                      int start_value,
                      int current_value,
                      vector<bool> &reachable) {
    if (!reachable[current_value]) {
        reachable[current_value] = true;
        for (int successor_value : dtg[current_value])
            recurse_forwards(dtg, start_value, successor_value, reachable);
    }
}

void StubbornSetsEC::initialize() {
    compute_sorted_operators();
    compute_operator_preconditions();
    compute_achievers();
    compute_conflicts_and_disabling();
    build_reachability_map();

    int num_variables = g_variable_domain.size();
    for (int var = 0; var < num_variables; var++) {
        nes_computed.push_back(vector<bool>(g_variable_domain[var], false));
    }

    cout << "partial order reduction method: stubborn sets ec" << endl;
}

void StubbornSetsEC::compute_operator_preconditions() {
    int num_operators = g_operators.size();
    int num_variables = g_variable_domain.size();
    operator_preconditions.resize(num_operators);
    for (int op_no = 0; op_no < num_operators; op_no++) {
        operator_preconditions[op_no].resize(num_variables, -1);
        const GlobalOperator &op = g_operators[op_no];
        for (const GlobalCondition &precondition : op.get_preconditions()) {
            operator_preconditions[op_no][var] = precondition.val;
        }
    }
}

void StubbornSetsEC::build_reachability_map() {
    vector<StubbornDTG> dtgs = build_dtgs();
    int num_variables = g_variable_domain.size();
    reachability_map.resize(num_variables);
    for (int var = 0; var < num_variables; ++var) {
        StubbornDTG &dtg = dtgs[var];
        int num_values = dtg.size();
        reachability_map[var].resize(num_values);
        for (int val = 0; val < num_values; ++val) {
            reachability_map[var][val].assign(num_values, false);
        }
        for (int start_value = 0; start_value < g_variable_domain[var]; start_value++) {
            vector<bool> &reachable = reachability_map[var][start_value];
            recurse_forwards(dtg, start_value, start_value, reachable);
        }
    }
}

void StubbornSetsEC::compute_active_operators(const GlobalState &state) {
    int num_operators = g_operators.size();
    for (int op_no = 0; op_no < num_operators; ++op_no) {
        const GlobalOperator &op = g_operators[op_no];
        bool all_preconditions_are_active = true;

        for (const GlobalCondition &precondition : op.get_preconditions()) {
            int var = precondition.var;
            int value = precondition.val;
            int current_value = state[var];
            const vector<bool> &reachable_values = reachability_map[var][current_value];
            if (!reachable_values[value]) {
                all_preconditions_are_active = false;
                break;
            }
        }

        if (all_preconditions_are_active) {
            active_ops[op_no] = true;
        }
    }
}

void StubbornSetsEC::compute_conflicts_and_disabling() {
    int num_operators = g_operators.size();
    conflicting_and_disabling.resize(num_operators);
    disabled.resize(num_operators);

    for (int op1_no = 0; op1_no < num_operators; ++op1_no) {
        for (int op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no) {
                bool conflict = can_conflict(op1_no, op2_no);
                bool disable = can_disable(op2_no, op1_no);
                if (conflict || disable) {
                    conflicting_and_disabling[op1_no].push_back(op2_no);
                }
                if (disable) {
                    disabled[op2_no].push_back(op1_no);
                }
            }
        }
    }
}

/* TODO: Currently, this is adapted from SimpleStubbornSets. We need
   to separate the functionality of marking stubborn operators (and
   move "mark_as_stubborn" to the stubborn sets base class) and the
   marking of written variables */
void StubbornSetsEC::mark_as_stubborn(int op_no, const GlobalState &state) {
    if (!stubborn[op_no]) {
        stubborn[op_no] = true;
        stubborn_queue.push_back(op_no);

        const GlobalOperator &op = g_operators[op_no];
        if (op.is_applicable(state)) {
            for (const GlobalEffect &effect : op.get_effects()) {
                written_vars[effect.var] = true;
            }
        }
    }
}

void StubbornSetsEC::add_nes_for_fact(Fact fact, const GlobalState &state) {
    for (int achiever : achievers[fact.var][fact.val]) {
        if (active_ops[achiever]) {
            mark_as_stubborn(achiever, state);
        }
    }

    nes_computed[fact.var][fact.val] = true;
}

void StubbornSetsEC::add_conflicting_and_disabling(int op_no,
                                                   const GlobalState &state) {
    for (int conflict : conflicting_and_disabling[op_no]) {
        if (active_ops[conflict])
            mark_as_stubborn(conflict, state);
    }
}

void StubbornSetsEC::get_disabled_vars(int op1_no,
                                       int op2_no,
                                       vector<int> &disabled_vars) {
    disabled_vars.clear();
    int i = 0;
    int j = 0;
    int num_op1_effects = op_effects[op1_no].size();
    int num_op2_preconditions = op_preconditions[op2_no].size();
    while (i < num_op2_preconditions && j < num_op1_effects) {
        int read_var = op_preconditions[op2_no][i].var;
        int write_var = op_effects[op1_no][j].var;
        if (read_var < write_var) {
            ++i;
        } else if (read_var > write_var) {
            ++j;
        } else {
            // read_var == write_var
            int read_value = op_preconditions[op2_no][i].val;
            int write_value = op_effects[op1_no][j].val;
            if (read_value != write_value) {
                disabled_vars.push_back(read_var);
            }
            ++i;
            ++j;
        }
    }
}

void StubbornSetsEC::apply_s5(const GlobalOperator &op, const GlobalState &state) {
    // Find a violated state variable and check if stubborn contains a writer for this variable.
    Fact violated_precondition(-1, -1);
    for (const GlobalCondition &precondition : op.get_preconditions()) {
        int var = precondition.var;
        int value = precondition.val;

        if (state[var] != value) {
            if (written_vars[var]) {
                if (!nes_computed[var][value]) {
                    add_nes_for_fact(Fact(var, value), state);
                }
                return;
            }
            if (violated_precondition.var == -1) {
                violated_precondition = Fact(var, value);
            }
        }
    }

    assert(violated_precondition.var != -1);
    if (!nes_computed[violated_precondition.var][violated_precondition.val]) {
        add_nes_for_fact(violated_precondition, state);
    }
}

void StubbornSetsEC::compute_stubborn_set(
    const GlobalState &state, vector<const GlobalOperator *> &applicable_ops) {
    stubborn.clear();
    stubborn.assign(g_operators.size(), false);
    active_ops.clear();
    active_ops.assign(g_operators.size(), false);

    for (size_t i = 0; i < nes_computed.size(); i++) {
        nes_computed[i].clear();
        nes_computed[i].assign(g_variable_domain[i], false);
    }

    compute_active_operators(state);
    written_vars.assign(g_variable_domain.size(), false);
    std::vector<int> disabled_vars;

    //rule S1
    Fact unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal.var != -1);
    add_nes_for_fact(unsatisfied_goal, state);     // active operators used

    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();
        stubborn_queue.pop_back();

        const GlobalOperator &op = g_operators[op_no];
        if (op.is_applicable(state)) {
            //Rule S2 & S3
            add_conflicting_and_disabling(op_no, state);     // active operators used
            //Rule S4'
            for (int disabled_op_no : disabled[op_no]) {
                if (active_ops[disabled_op_no]) {
                    bool v_applicable_op_found = false;
                    get_disabled_vars(op_no, disabled_op_no, disabled_vars);
                    if (!disabled_vars.empty()) {     // == can_disable(op1_no, op2_no)
                        for (int disabled_var : disabled_vars) {
                            //First case: add o'
                            if (is_v_applicable(disabled_var,
                                                disabled_op_no,
                                                state,
                                                operator_preconditions)) {
                                mark_as_stubborn(disabled_op_no, state);
                                v_applicable_op_found = true;
                                break;
                            }
                        }

                        //Second case: add a necessray enabling set for o' following S5
                        if (!v_applicable_op_found) {
                            apply_s5(g_operators[disabled_op_no], state);
                        }
                    }
                }
            }
        } else {     // op is inapplicable
            //S5
            apply_s5(op, state);
        }
    }

    // Now check which applicable operators are in the stubborn set.
    vector<const GlobalOperator *> remaining_ops;
    remaining_ops.reserve(applicable_ops.size());
    for (const GlobalOperator *op : applicable_ops) {
        int op_no = get_op_index(op);
        if (stubborn[op_no])
            remaining_ops.push_back(op);
    }
    if (remaining_ops.size() != applicable_ops.size()) {
        applicable_ops.swap(remaining_ops);
        sort(applicable_ops.begin(), applicable_ops.end());
    }
}

static shared_ptr<PORMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "StubbornSetsEC",
        "stubborn set method that dominates expansion core");

    return make_shared<StubbornSetsEC>();
}

static PluginShared<PORMethod> _plugin("stubborn_sets_ec", _parse);
}
