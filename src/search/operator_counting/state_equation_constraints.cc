#include "state_equation_constraints.h"

#include "../globals.h"
#include "../lp_solver.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

using namespace std;

namespace operator_counting {

void add_indices_to_constraint(LPConstraint &constraint,
                                const set<int> &indices,
                                double coefficient) {
    for (int index : indices) {
        constraint.insert(index, coefficient);
    }
}


void StateEquationConstraints::build_propositions(const TaskProxy &task_proxy) {
    VariablesProxy vars = task_proxy.get_variables();
    propositions.reserve(vars.size());
    for (VariableProxy var : vars) {
        propositions.push_back(vector<Proposition>(var.get_domain_size()));
    }
    OperatorsProxy ops = task_proxy.get_operators();
    for (size_t op_id = 0; op_id < ops.size(); ++op_id) {
        const OperatorProxy &op = ops[op_id];
        vector<int> precondition(vars.size(), -1);
        for (FactProxy condition : op.get_preconditions()) {
            int pre_var_id = condition.get_variable().get_id();
            precondition[pre_var_id] = condition.get_value();
        }
        for (EffectProxy effect_proxy : op.get_effects()) {
            FactProxy effect = effect_proxy.get_fact();
            int var = effect.get_variable().get_id();
            int pre = precondition[var];
            int post = effect.get_value();
            assert(post != -1);
            assert(pre != post);

            if (pre != -1) {
                propositions[var][post].always_produced_by.insert(op_id);
                propositions[var][pre].always_consumed_by.insert(op_id);
            } else {
                propositions[var][post].sometimes_produced_by.insert(op_id);
            }
        }
    }
}

void StateEquationConstraints::add_constraints(
    vector<LPConstraint> &constraints, double infinity) {
    for (vector<Proposition> &var_propositions : propositions) {
        for (Proposition &prop : var_propositions) {
            LPConstraint constraint(-infinity, infinity);
            add_indices_to_constraint(constraint, prop.always_produced_by, 1.0);
            add_indices_to_constraint(constraint, prop.sometimes_produced_by, 1.0);
            add_indices_to_constraint(constraint, prop.always_consumed_by, -1.0);
            if (!constraint.empty()) {
                prop.constraint_index = constraints.size();
                constraints.push_back(constraint);
            }
        }
    }
}

void StateEquationConstraints::initialize_constraints(
    const shared_ptr<AbstractTask> task, vector<LPConstraint> &constraints,
    double infinity) {
    cout << "Initializing constraints from state equation." << endl;
    TaskProxy task_proxy(*task);
    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);
    build_propositions(task_proxy);
    add_constraints(constraints, infinity);

    // Initialize goal state.
    VariablesProxy variables = task_proxy.get_variables();
    goal_state = vector<int>(variables.size(), numeric_limits<int>::max());
    for (FactProxy goal : task_proxy.get_goals()) {
        goal_state[goal.get_variable().get_id()] = goal.get_value();

    }
}

bool StateEquationConstraints::update_constraints(const State &state,
                                                  LPSolver &lp_solver) {
    // Compute the bounds for the rows in the LP.
    for (size_t var = 0; var < propositions.size(); ++var) {
        int num_values = propositions[var].size();
        for (int value = 0; value < num_values; ++value) {
            const Proposition &prop = propositions[var][value];
            // Set row bounds.
            if (prop.constraint_index >= 0) {
                double lower_bound = 0;
                /* If we consider the current value of var, there must be an
                   additional consumer. */
                if (state[var].get_value() == value) {
                    --lower_bound;
                }
                /* If we consider the goal value of var, there must be an
                   additional producer. */
                if (goal_state[var] == value) {
                    ++lower_bound;
                }
                lp_solver.set_constraint_lower_bound(
                    prop.constraint_index, lower_bound);
            }
        }
    }
    return false;
}

static shared_ptr<ConstraintGenerator> _parse(OptionParser &parser) {
    if (parser.dry_run())
        return nullptr;
    return make_shared<StateEquationConstraints>();
}

PluginShared<ConstraintGenerator> _plugin("state_equation_constraints", _parse);
}
