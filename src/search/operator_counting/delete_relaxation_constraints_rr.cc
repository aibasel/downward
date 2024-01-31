#include "delete_relaxation_constraints_rr.h"

#include "../lp/lp_solver.h"
#include "../plugins/plugin.h"
#include "../task_proxy.h"
#include "../utils/markup.h"

#include <cassert>

using namespace std;

namespace operator_counting {
static void add_lp_variables(int count, LPVariables &variables,
                             vector<int> &indices, double lower, double upper,
                             double objective, bool is_integer) {
    for (int i = 0; i < count; ++i) {
        indices.push_back(variables.size());
        variables.emplace_back(lower, upper, objective, is_integer);
    }
}

DeleteRelaxationConstraintsRR::DeleteRelaxationConstraintsRR(
    const plugins::Options &opts)
    : use_time_vars(opts.get<bool>("use_time_vars")),
      use_integer_vars(opts.get<bool>("use_integer_vars")) {}

int DeleteRelaxationConstraintsRR::get_var_f_defined(FactPair f) {
    return lp_var_id_f_defined[f.var][f.value];
}

int DeleteRelaxationConstraintsRR::get_var_f_maps_to(FactPair f,
                                                     const OperatorProxy &op) {
    return lp_var_id_f_maps_to.at(make_tuple(f.var, f.value, op.get_id()));
}

bool DeleteRelaxationConstraintsRR::is_in_effect(FactPair f,
                                                 const OperatorProxy &op) {
    for (EffectProxy eff : op.get_effects()) {
        if (eff.get_fact().get_pair() == f) {
            return true;
        }
    }
    return false;
}

bool DeleteRelaxationConstraintsRR::is_in_precondition(
    FactPair f, const OperatorProxy &op) {
    for (FactProxy pre : op.get_preconditions()) {
        if (pre.get_pair() == f) {
            return true;
        }
    }
    return false;
}

void DeleteRelaxationConstraintsRR::create_auxiliary_variables(
    const TaskProxy &task_proxy, LPVariables &variables) {
    OperatorsProxy ops = task_proxy.get_operators();
    int num_ops = ops.size();
    VariablesProxy vars = task_proxy.get_variables();
    int num_vars = vars.size();

    lp_var_id_f_defined.resize(num_vars);
    for (VariableProxy var : vars) {
        int var_id = var.get_id();
        // Add f_p variable.
        add_lp_variables(var.get_domain_size(), variables,
                         lp_var_id_f_defined[var_id], 0, 1, 0,
                         use_integer_vars);
        // Add f_{p,a} variables.
        for (int value = 0; value < var.get_domain_size(); ++value) {
            for (OperatorProxy op : ops) {
                if (is_in_effect(FactPair(var_id, value), op)) {
                    lp_var_id_f_maps_to.emplace(
                        make_pair(make_tuple(var_id, value, op.get_id()),
                                  variables.size()));
                    variables.emplace_back(0, 1, 0, use_integer_vars);
                }
            }
        }
    }
}

void DeleteRelaxationConstraintsRR::create_constraints(
    const TaskProxy &task_proxy, lp::LinearProgram &lp) {
    LPVariables &variables = lp.get_variables();
    LPConstraints &constraints = lp.get_constraints();
    double infinity = lp.get_infinity();
    OperatorsProxy ops = task_proxy.get_operators();
    VariablesProxy vars = task_proxy.get_variables();

    /*
      f must map a proposition to at most one operator.
      Constraint (2) in paper.
    */
    for (VariableProxy var_p : vars) {
        for (int value_p = 0; value_p < var_p.get_domain_size(); ++value_p) {
            constraints.emplace_back(0, 0);
            FactPair fact_p(var_p.get_id(), value_p);
            constraints.back().insert(get_var_f_defined(fact_p), -1);
            for (OperatorProxy op : ops) {
                if (is_in_effect(fact_p, op))
                    constraints.back().insert(get_var_f_maps_to(fact_p, op), 1);
            }
        }
    }

    /*
      Constraint (3) in paper.
    */
    for (VariableProxy var_p : vars) {
        for (int value_p = 0; value_p < var_p.get_domain_size(); ++value_p) {
            FactPair fact_p(var_p.get_id(), value_p);
            for (VariableProxy var_q : vars) {
                for (int value_q = 0; value_q < var_q.get_domain_size();
                     ++value_q) {
                    FactPair fact_q(var_q.get_id(), value_q);
                    constraints.emplace_back(0, 1);
                    constraints.back().insert(get_var_f_defined(fact_q), 1);
                    for (OperatorProxy op : ops) {
                        if (is_in_precondition(fact_q, op) &&
                            is_in_effect(fact_p, op)) {
                            constraints.back().insert(
                                get_var_f_maps_to(fact_p, op), -1);
                        }
                    }
                }
            }
        }
    }

    /*
      Constraint (4) in paper.
    */
    for (FactProxy goal : task_proxy.get_goals()) {
        variables[get_var_f_defined(goal.get_pair())].lower_bound = 1;
    }

    /*
      Constraint (5) in paper.
    */
    for (OperatorProxy op : ops) {
        for (EffectProxy eff : op.get_effects()) {
            FactPair fact_p = eff.get_fact().get_pair();
            constraints.emplace_back(0, infinity);
            constraints.back().insert(get_var_f_maps_to(fact_p, op), -1);
            constraints.back().insert(op.get_id(), 1);
        }
    }
}

void DeleteRelaxationConstraintsRR::initialize_constraints(
    const shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) {
    TaskProxy task_proxy(*task);
    create_auxiliary_variables(task_proxy, lp.get_variables());
    create_constraints(task_proxy, lp);
}

bool DeleteRelaxationConstraintsRR::update_constraints(
    const State &state, lp::LPSolver &lp_solver) {
    // Unset old bounds.
    for (FactPair f : last_state) {
        lp_solver.set_constraint_lower_bound(get_constraint_id(f), 0);
    }
    last_state.clear();
    // Set new bounds.
    for (FactProxy f : state) {
        lp_solver.set_constraint_lower_bound(get_constraint_id(f.get_pair()),
                                             -1);
        last_state.push_back(f.get_pair());
    }
    return false;
}

class DeleteRelaxationConstraintsRRFeature
    : public plugins::TypedFeature<ConstraintGenerator,
                                   DeleteRelaxationConstraintsRR> {
public:
    DeleteRelaxationConstraintsRRFeature()
        : TypedFeature("delete_relaxation_constraints_rr") {
        document_title(
            "Delete relaxation constraints from Rankooh and Rintanen");
        document_synopsis(
            "Operator-counting constraints based on the delete relaxation. By "
            "default the constraints encode an easy-to-compute relaxation of "
            "h^+^. "
            "With the right settings, these constraints can be used to compute "
            "the "
            "optimal delete-relaxation heuristic h^+^ (see example below). "
            "For details, see" +
            utils::format_journal_reference(
                {"Masood Feyzbakhsh Rankooh", "Jussi Rintanen"},
                "Efficient Computation and Informative Estimation of"
                "h+ by Integer and Linear Programming"
                "",
                "https://ojs.aaai.org/index.php/ICAPS/article/view/19787/19546",
                "Proceedings of the Thirty-Second International Conference on "
                "Automated Planning and Scheduling (ICAPS2022)",
                "32", "71-79", "2022"));

        add_option<bool>(
            "use_time_vars",
            "use variables for time steps. With these additional variables the "
            "constraints enforce an order between the selected operators.",
            "false");
        add_option<bool>(
            "use_integer_vars",
            "restrict auxiliary variables to integer values. These variables "
            "encode whether operators are used, facts are reached, which "
            "operator "
            "first achieves which fact, and in which order the operators are "
            "used. "
            "Restricting them to integers generally improves the heuristic "
            "value "
            "at the cost of increased runtime.",
            "false");

        document_note(
            "Example",
            "To compute the optimal delete-relaxation heuristic h^+^, use\n"
            "{{{\noperatorcounting([delete_relaxation_constraints_rr(use_time_"
            "vars=true, "
            "use_integer_vars=true)], "
            "use_integer_operator_counts=true))\n}}}\n");
    }
};

static plugins::FeaturePlugin<DeleteRelaxationConstraintsRRFeature> _plugin;
}
