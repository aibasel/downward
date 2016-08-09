#include "stubborn_sets_ec.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"

#include <cassert>

using namespace std;

namespace stubborn_sets_ec {
// DTGs are stored as one adjacency list per value.
using StubbornDTG = vector<vector<int>>;

static inline bool is_v_applicable(int var,
                                   int op_no,
                                   const GlobalState &state,
                                   vector<vector<int>> &preconditions) {
    int precondition_on_var = preconditions[op_no][var];
    return precondition_on_var == -1 || precondition_on_var == state[var];
}

// Copied from SimpleStubbornSets
static inline FactPair find_unsatisfied_goal(const GlobalState &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        int goal_value = g_goal[i].second;
        if (state[goal_var] != goal_value)
            return FactPair(goal_var, goal_value);
    }
    return FactPair(-1, -1);
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

// Relies on both fact sets being sorted by variable.
void get_conflicting_vars(const vector<FactPair> &facts1,
                          const vector<FactPair> &facts2,
                          vector<int> &conflicting_vars) {
    conflicting_vars.clear();
    auto facts1_it = facts1.begin();
    auto facts2_it = facts2.begin();
    while (facts1_it != facts1.end() &&
           facts2_it != facts2.end()) {
        if (facts1_it->var < facts2_it->var) {
            ++facts1_it;
        } else if (facts1_it->var > facts2_it->var) {
            ++facts2_it;
        } else {
            if (facts2_it->value != facts1_it->value) {
                conflicting_vars.push_back(facts2_it->var);
            }
            ++facts1_it;
            ++facts2_it;
        }
    }
}

StubbornSetsEC::StubbornSetsEC() {
    compute_operator_preconditions();
    compute_conflicts_and_disabling();
    build_reachability_map();

    int num_variables = g_variable_domain.size();
    for (int var = 0; var < num_variables; var++) {
        nes_computed.push_back(vector<bool>(g_variable_domain[var], false));
    }

    cout << "pruning method: stubborn sets ec" << endl;
}

void StubbornSetsEC::compute_operator_preconditions() {
    int num_operators = g_operators.size();
    int num_variables = g_variable_domain.size();
    op_preconditions_on_var.resize(num_operators);
    for (int op_no = 0; op_no < num_operators; op_no++) {
        op_preconditions_on_var[op_no].resize(num_variables, -1);
        const GlobalOperator &op = g_operators[op_no];
        for (const GlobalCondition &precondition : op.get_preconditions()) {
            op_preconditions_on_var[op_no][precondition.var] = precondition.val;
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

// TODO: find a better name.
void StubbornSetsEC::mark_as_stubborn_and_remember_written_vars(
    int op_no, const GlobalState &state) {
    if (mark_as_stubborn(op_no)) {
        const GlobalOperator &op = g_operators[op_no];
        if (op.is_applicable(state)) {
            for (const GlobalEffect &effect : op.get_effects()) {
                written_vars[effect.var] = true;
            }
        }
    }
}

/* TODO: think about a better name, which distinguishes this method
   better from the corresponding method for simple stubborn sets */
void StubbornSetsEC::add_nes_for_fact(const FactPair &fact, const GlobalState &state) {
    for (int achiever : achievers[fact.var][fact.value]) {
        if (active_ops[achiever]) {
            mark_as_stubborn_and_remember_written_vars(achiever, state);
        }
    }

    nes_computed[fact.var][fact.value] = true;
}

void StubbornSetsEC::add_conflicting_and_disabling(int op_no,
                                                   const GlobalState &state) {
    for (int conflict : conflicting_and_disabling[op_no]) {
        if (active_ops[conflict])
            mark_as_stubborn_and_remember_written_vars(conflict, state);
    }
}

// Relies on op_effects and op_preconditions being sorted by variable.
void StubbornSetsEC::get_disabled_vars(
    int op1_no, int op2_no, vector<int> &disabled_vars) {
    get_conflicting_vars(sorted_op_effects[op1_no],
                         sorted_op_preconditions[op2_no],
                         disabled_vars);
}

void StubbornSetsEC::apply_s5(const GlobalOperator &op, const GlobalState &state) {
    // Find a violated state variable and check if stubborn contains a writer for this variable.
    FactPair violated_precondition(-1, -1);
    for (const GlobalCondition &precondition : op.get_preconditions()) {
        int var = precondition.var;
        int value = precondition.val;

        if (state[var] != value) {
            if (written_vars[var]) {
                if (!nes_computed[var][value]) {
                    add_nes_for_fact(FactPair(var, value), state);
                }
                return;
            }
            if (violated_precondition.var == -1) {
                violated_precondition = FactPair(var, value);
            }
        }
    }

    assert(violated_precondition.var != -1);
    if (!nes_computed[violated_precondition.var][violated_precondition.value]) {
        add_nes_for_fact(violated_precondition, state);
    }
}

void StubbornSetsEC::initialize_stubborn_set(const GlobalState &state) {
    active_ops.clear();
    active_ops.assign(g_operators.size(), false);
    for (size_t i = 0; i < nes_computed.size(); i++) {
        nes_computed[i].clear();
        nes_computed[i].assign(g_variable_domain[i], false);
    }
    written_vars.assign(g_variable_domain.size(), false);

    compute_active_operators(state);

    //rule S1
    FactPair unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal.var != -1);
    add_nes_for_fact(unsatisfied_goal, state);     // active operators used
}

void StubbornSetsEC::handle_stubborn_operator(const GlobalState &state, int op_no) {
    const GlobalOperator &op = g_operators[op_no];
    if (op.is_applicable(state)) {
        //Rule S2 & S3
        add_conflicting_and_disabling(op_no, state);     // active operators used
        //Rule S4'
        vector<int> disabled_vars;
        for (int disabled_op_no : disabled[op_no]) {
            if (active_ops[disabled_op_no]) {
                get_disabled_vars(op_no, disabled_op_no, disabled_vars);
                if (!disabled_vars.empty()) {     // == can_disable(op1_no, op2_no)
                    bool v_applicable_op_found = false;
                    for (int disabled_var : disabled_vars) {
                        //First case: add o'
                        if (is_v_applicable(disabled_var,
                                            disabled_op_no,
                                            state,
                                            op_preconditions_on_var)) {
                            mark_as_stubborn_and_remember_written_vars(disabled_op_no, state);
                            v_applicable_op_found = true;
                            break;
                        }
                    }

                    //Second case: add a necessary enabling set for o' following S5
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

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "StubbornSetsEC",
        "Stubborn sets represent a state pruning method which computes a subset "
        "of applicable operators in each state such that completeness and "
        "optimality of the overall search is preserved. As stubborn sets rely "
        "on several design choices, there are different variants thereof. "
        "The variant 'StubbornSetsEC' resolves the design choices such that "
        "the resulting pruning method is guaranteed to strictly dominate the "
        "Expansion Core pruning method. For details, see" + utils::format_paper_reference(
            {"Martin Wehrle", "Malte Helmert", "Yusra Alkhazraji", "Robert Mattmueller"},
            "The Relative Pruning Power of Strong Stubborn Sets and Expansion Core",
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS13/paper/view/6053/6185",
            "Proceedings of the 23rd International Conference on Automated Planning "
            "and Scheduling (ICAPS 2013)",
            "251-259",
            "AAAI Press 2013"));

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsEC>();
}

static PluginShared<PruningMethod> _plugin("stubborn_sets_ec", _parse);
}
