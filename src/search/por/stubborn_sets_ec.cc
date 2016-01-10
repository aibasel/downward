#include "stubborn_sets_ec.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace StubbornSetsEC {
// TODO: needs a central place (see comment for simple stubborn sets)
static inline int get_op_index(const GlobalOperator *op) {
    int op_index = op - &*g_operators.begin();
    assert(op_index >= 0 && (uint)op_index < g_operators.size());
    return op_index;
}

static inline bool is_v_applicable(int var, int op_no, const GlobalState &state, std::vector<std::vector<int>> &v_precond) {
    int vprecond = v_precond[op_no][var];
    return vprecond == -1 || vprecond == state[var];
}

// Copied from SimpleStubbornSets
static inline pair<int, int> find_unsatisfied_goal(const GlobalState &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        int goal_value = g_goal[i].second;
        if (state[goal_var] != goal_value)
            return make_pair(goal_var, goal_value);
    }
    return make_pair(-1, -1);
}

StubbornSetsEC::StubbornSetsEC() {}

StubbornSetsEC::~StubbornSetsEC() {}

void StubbornSetsEC::initialize() {
    compute_sorted_operators();
    compute_v_precond();
    compute_achievers();
    compute_conflicts_and_disabling();
    size_t num_variables = g_variable_domain.size();
    reachability_map.resize(num_variables);
    build_dtgs();
    build_reachability_map();

    for (size_t i = 0; i < g_variable_domain.size(); i++) {
        nes_computed.push_back(vector<bool>(g_variable_domain[i], false));
    }

    cout << "partial order reduction method: stubborn sets ec" << endl;
}

void StubbornSetsEC::build_dtgs() {
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

    assert(dtgs.empty());
    size_t num_variables = g_variable_domain.size();

    dtgs.resize(num_variables);

    // Step 1: Create the empty DTG nodes.
    for (uint var_no = 0; var_no < num_variables; ++var_no) {
        size_t var_size = g_variable_domain[var_no];
        dtgs[var_no].nodes.resize(var_size);
        dtgs[var_no].goal_values.resize(var_size, true);
    }

    // Step 2: Mark goal values in each DTG. Variables that are not in
    //         the goal have previously been set to "all are goals".
    for (uint i = 0; i < g_goal.size(); ++i) {
        int var_no = g_goal[i].first;
        int goal_value = g_goal[i].second;
        vector<bool> &goal_values = dtgs[var_no].goal_values;
        size_t var_size = g_variable_domain[var_no];
        goal_values.clear();
        goal_values.resize(var_size, false);
        goal_values[goal_value] = true;
    }

    // Step 3: Add DTG arcs.
    for (uint op_no = 0; op_no < g_operators.size(); ++op_no) {
        const GlobalOperator &op = g_operators[op_no];

        const vector<GlobalEffect> &effects = op.get_effects();
        for (uint i = 0; i < effects.size(); ++i) {
            int eff_var = effects[i].var;
            int eff_val = effects[i].val;

            int pre_var = -1;
            int pre_val = -1;

            const vector<GlobalCondition> &preconds = op.get_preconditions();
            for (uint j = 0; j < preconds.size(); j++) {
                if (preconds[j].var == eff_var) {
                    pre_var = preconds[j].var;
                    pre_val = preconds[j].val;
                    break;
                }
            }

            StubbornDTG &dtg = dtgs[eff_var];
            int pre_value_min, pre_value_max;
            if (pre_var == -1) {
                pre_value_min = 0;
                pre_value_max = g_variable_domain[eff_var];
            } else {
                pre_value_min = pre_val;
                pre_value_max = pre_val + 1;
            }

            for (int value = pre_value_min; value < pre_value_max; ++value) {
                dtg.nodes[value].outgoing.push_back(StubbornDTG::Arc(eff_val, op_no));
                dtg.nodes[eff_val].incoming.push_back(StubbornDTG::Arc(value, op_no));
            }
        }
    }
}

void StubbornSetsEC::compute_v_precond() {
    v_precond.resize(g_operators.size());
    for (uint op_no = 0; op_no < g_operators.size(); op_no++) {
        v_precond[op_no].resize(g_variable_name.size(), -1);
        for (uint var = 0; var < g_variable_name.size(); var++) {
            const vector<GlobalCondition> &preconds = g_operators[op_no].get_preconditions();
            for (size_t i = 0; i < preconds.size(); i++) {
                if (preconds[i].var == (int)var) {
                    v_precond[op_no][var] = preconds[i].val;
                }
            }
        }
    }
}

void StubbornSetsEC::build_reachability_map() {
    size_t num_variables = g_variable_domain.size();
    for (uint var_no = 0; var_no < num_variables; ++var_no) {
        StubbornDTG &dtg = dtgs[var_no];
        size_t num_values = dtg.nodes.size();
        reachability_map[var_no].resize(num_values);
        for (uint val = 0; val < num_values; ++val) {
            reachability_map[var_no][val].assign(num_values, false);
        }
        for (int start_value = 0; start_value < g_variable_domain[var_no]; start_value++) {
            vector<bool> &reachable = reachability_map[var_no][start_value];
            recurse_forwards(var_no, start_value, start_value, reachable);
        }
    }
}


void StubbornSetsEC::recurse_forwards(int var, int start_value, int current_value, std::vector<bool> &reachable) {
    StubbornDTG &dtg = dtgs[var];
    if (!reachable[current_value]) {
        reachable[current_value] = true;
        const vector<StubbornDTG::Arc> &outgoing = dtg.nodes[current_value].outgoing;
        for (uint i = 0; i < outgoing.size(); ++i)
            recurse_forwards(var, start_value, outgoing[i].target_value, reachable);
    }
}

void StubbornSetsEC::compute_active_operators(const GlobalState &state) {
    for (size_t op_no = 0; op_no < g_operators.size(); ++op_no) {
        GlobalOperator &op = g_operators[op_no];
        bool all_preconditions_are_active = true;

        const vector<GlobalCondition> &preconds = op.get_preconditions();
        for (size_t i = 0; i < preconds.size(); i++) {
            int var = preconds[i].var;
            int value = preconds[i].val;
            int current_value = state[var];
            vector<bool> &reachable_values = reachability_map[var][current_value];
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
    size_t num_operators = g_operators.size();
    conflicting_and_disabling.resize(num_operators);
    disabled.resize(num_operators);

    for (size_t op1_no = 0; op1_no < num_operators; ++op1_no) {
        for (size_t op2_no = 0; op2_no < num_operators; ++op2_no) {
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
            const vector<GlobalEffect> &effects = op.get_effects();
            for (size_t i = 0; i < effects.size(); ++i) {
                int var = effects[i].var;
                written_vars[var] = true;
            }
        }
    }
}

void StubbornSetsEC::add_nes_for_fact(pair<int, int> fact, const GlobalState &state) {
    int var = fact.first;
    int value = fact.second;
    const vector<int> &op_nos = achievers[var][value];
    for (size_t i = 0; i < op_nos.size(); ++i) {
        if (active_ops[op_nos[i]]) {
            mark_as_stubborn(op_nos[i], state);
        }
    }

    nes_computed[var][value] = true;
}

void StubbornSetsEC::add_conflicting_and_disabling(int op_no, const GlobalState &state) {
    const vector<int> &conflict_and_disable = conflicting_and_disabling[op_no];

    for (size_t i = 0; i < conflict_and_disable.size(); ++i) {
        if (active_ops[conflict_and_disable[i]])
            mark_as_stubborn(conflict_and_disable[i], state);
    }
}

void StubbornSetsEC::get_disabled_vars(int op1_no, int op2_no, std::vector<int> &disabled_vars) {
    disabled_vars.clear();
    size_t i = 0;
    size_t j = 0;
    while (i < op_preconds[op2_no].size() && j < op_effects[op1_no].size()) {
        int read_var = op_preconds[op2_no][i].var;
        int write_var = op_effects[op1_no][j].var;
        if (read_var < write_var) {
            i++;
        } else {
            if (read_var == write_var) {
                int read_value = op_preconds[op2_no][i].val;
                int write_value = op_effects[op1_no][j].val;
                if (read_value != write_value) {
                    disabled_vars.push_back(read_var);
                }
                i++;
                j++;
            } else {
                // read_var > write_var
                j++;
            }
        }
    }
}

void StubbornSetsEC::apply_s5(const GlobalOperator &op, const GlobalState &state) {
    // Find a violated state variable and check if stubborn contains a writer for this variable.

    std::pair<int, int> violated_precondition = make_pair(-1, -1);
    std::pair<int, int> violated_pre_post = make_pair(-1, -1);

    const vector<GlobalCondition> &preconds = op.get_preconditions();
    for (size_t i = 0; i < preconds.size(); i++) {
        int var = preconds[i].var;
        int value = preconds[i].val;

        if (state[var] != value) {
            if (written_vars[var]) {
                if (!nes_computed[var][value]) {
                    std::pair<int, int> fact = make_pair(var, value);
                    add_nes_for_fact(fact, state);
                }
                return;
            }
            if (violated_precondition.first == -1) {
                violated_precondition = make_pair(var, value);
            }
        }
    }

    if (violated_pre_post.first != -1) {
        if (!nes_computed[violated_pre_post.first][violated_pre_post.second]) {
            add_nes_for_fact(violated_pre_post, state);
        }
        return;
    }

    assert(violated_precondition.first != -1);
    if (!nes_computed[violated_precondition.first][violated_precondition.second]) {
        add_nes_for_fact(violated_precondition, state);
    }

    return;
}


void StubbornSetsEC::compute_stubborn_set(const GlobalState &state, std::vector<const GlobalOperator *> &applicable_ops) {
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
    pair<int, int> goal_pair = find_unsatisfied_goal(state);
    assert(goal_pair.first != -1);
    add_nes_for_fact(goal_pair, state);     // active operators used

    while (!stubborn_queue.empty()) {
        int op_no = stubborn_queue.back();

        stubborn_queue.pop_back();
        const GlobalOperator &op = g_operators[op_no];
        if (op.is_applicable(state)) {
            //Rule S2 & S3
            add_conflicting_and_disabling(op_no, state);     // active operators used
            //Rule S4'
            std::vector<int>::iterator op_it;
            for (op_it = disabled[op_no].begin(); op_it != disabled[op_no].end(); op_it++) {
                if (active_ops[*op_it]) {
                    const GlobalOperator &o = g_operators[*op_it];

                    bool v_applicable_op_found = false;
                    std::vector<int>::iterator var_it;
                    get_disabled_vars(op_no, *op_it, disabled_vars);
                    if (!disabled_vars.empty()) {     // == can_disable(op1_no, op2_no)
                        for (var_it = disabled_vars.begin(); var_it != disabled_vars.end(); var_it++) {
                            //First case: add o'
                            if (is_v_applicable(*var_it, *op_it, state, v_precond)) {
                                mark_as_stubborn(*op_it, state);
                                v_applicable_op_found = true;
                                break;
                            }
                        }

                        //Second case: add a necessray enabling set for o' following S5
                        if (!v_applicable_op_found) {
                            apply_s5(o, state);
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
    for (size_t i = 0; i < applicable_ops.size(); ++i) {
        const GlobalOperator *op = applicable_ops[i];
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
    parser.document_synopsis("StubbornSetsEC", "stubborn set method that dominates expansion core");

    return make_shared<StubbornSetsEC>();
}

static PluginShared<PORMethod> _plugin("stubborn_sets_ec", _parse);
}
