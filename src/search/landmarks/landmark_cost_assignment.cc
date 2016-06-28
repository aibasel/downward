#include "landmark_cost_assignment.h"

#include "landmark_graph.h"
#include "util.h"

#include "../utils/collections.h"
#include "../utils/language.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>

using namespace std;

namespace landmarks {
LandmarkCostAssignment::LandmarkCostAssignment(
    LandmarkGraph &graph, OperatorCost cost_type_)
    : lm_graph(graph), cost_type(cost_type_) {
}


LandmarkCostAssignment::~LandmarkCostAssignment() {
}


const set<int> &LandmarkCostAssignment::get_achievers(
    int lmn_status, const LandmarkNode &lmn) const {
    // Return relevant achievers of the landmark according to its status.
    if (lmn_status == lm_not_reached)
        return lmn.first_achievers;
    else if (lmn_status == lm_needed_again)
        return lmn.possible_achievers;
    else
        return empty;
}


// Uniform cost partioning
LandmarkUniformSharedCostAssignment::LandmarkUniformSharedCostAssignment(
    LandmarkGraph &graph, bool use_action_landmarks_, OperatorCost cost_type_)
    : LandmarkCostAssignment(graph, cost_type_), use_action_landmarks(use_action_landmarks_) {
}


LandmarkUniformSharedCostAssignment::~LandmarkUniformSharedCostAssignment() {
}


double LandmarkUniformSharedCostAssignment::cost_sharing_h_value(const TaskProxy &task_proxy) {
    int num_ops = task_proxy.get_operators().size();
    vector<int> achieved_lms_by_op(num_ops, 0);
    vector<bool> action_landmarks(num_ops, false);

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    set<LandmarkNode *>::iterator node_it;

    double h = 0;

    /* First pass:
       compute which op achieves how many landmarks. Along the way,
       mark action landmarks and add their cost to h. */
    for (const LandmarkNode *node : nodes) {
        int lmn_status = node->get_status();
        if (lmn_status != lm_reached) {
            const set<int> &achievers = get_achievers(lmn_status, *node);
            assert(!achievers.empty());
            if (use_action_landmarks && achievers.size() == 1) {
                // We have found an action landmark for this state.
                int op_id = *achievers.begin();
                if (!action_landmarks[op_id]) {
                    action_landmarks[op_id] = true;
                    OperatorProxy op = get_operator_or_axiom(task_proxy, op_id);
                    assert(!op.is_axiom());
                    h += op.get_cost();
                }
            } else {
                for (int op_id : achievers) {
                    assert(!get_operator_or_axiom(task_proxy, op_id).is_axiom());
                    ++achieved_lms_by_op[op_id];
                }
            }
        }
    }

    vector<LandmarkNode *> relevant_lms;

    /* Second pass:
       remove landmarks from consideration that are covered by
       an action landmark; decrease the counters accordingly
       so that no unnecessary cost is assigned to these landmarks. */
    for (LandmarkNode *node : nodes) {
        int lmn_status = node->get_status();
        if (lmn_status != lm_reached) {
            const set<int> &achievers = get_achievers(lmn_status, *node);
            bool covered_by_action_lm = false;
            for (int op_id : achievers) {
                assert(!get_operator_or_axiom(task_proxy, op_id).is_axiom());
                if (action_landmarks[op_id]) {
                    covered_by_action_lm = true;
                    break;
                }
            }
            if (covered_by_action_lm) {
                for (int op_id : achievers) {
                    assert(!get_operator_or_axiom(task_proxy, op_id).is_axiom());
                    --achieved_lms_by_op[op_id];
                }
            } else {
                relevant_lms.push_back(node);
            }
        }
    }

    /* Third pass:
       count shared costs for the remaining landmarks. */
    for (const LandmarkNode *node : relevant_lms) {
        int lmn_status = node->get_status();
        const set<int> &achievers = get_achievers(lmn_status, *node);
        double min_cost = numeric_limits<double>::max();
        for (int op_id : achievers) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_id);
            assert(!op.is_axiom());
            int num_achieved = achieved_lms_by_op[op_id];
            assert(num_achieved >= 1);
            double shared_cost = double(op.get_cost()) / num_achieved;
            min_cost = min(min_cost, shared_cost);
        }
        h += min_cost;
    }

    return h;
}

LandmarkEfficientOptimalSharedCostAssignment::LandmarkEfficientOptimalSharedCostAssignment(
    const TaskProxy &task_proxy, LandmarkGraph &graph,
    OperatorCost cost_type, lp::LPSolverType solver_type)
    : LandmarkCostAssignment(graph, cost_type),
      lp_solver(solver_type) {
    OperatorsProxy operators = task_proxy.get_operators();
    /* The LP has one variable (column) per landmark and one
       inequality (row) per operator. */
    int num_cols = lm_graph.number_of_landmarks();
    int num_rows = operators.size();

    /* We want to maximize 1 * cost(lm_1) + ... + 1 * cost(lm_n),
       so the coefficients are all 1.
       Variable bounds are state-dependent; we initialize the range to {0}. */
    lp_variables.resize(num_cols, lp::LPVariable(0.0, 0.0, 1.0));

    /* Set up lower bounds and upper bounds for the inequalities.
       These simply say that the operator's total cost must fall
       between 0 and the real operator cost. */
    lp_constraints.resize(num_rows, lp::LPConstraint(0.0, 0.0));
    for (OperatorProxy op : operators) {
        lp_constraints[op.get_id()].set_lower_bound(0);
        lp_constraints[op.get_id()].set_upper_bound(op.get_cost());
    }
}

LandmarkEfficientOptimalSharedCostAssignment::~LandmarkEfficientOptimalSharedCostAssignment() {
}

double LandmarkEfficientOptimalSharedCostAssignment::cost_sharing_h_value(const TaskProxy &task_proxy) {
    utils::unused_variable(task_proxy);
    /* TODO: We could also do the same thing with action landmarks we
             do in the uniform cost partitioning case. */

    /*
      Set up LP variable bounds for the landmarks.
      The range of cost(lm_1) is {0} if the landmark is already
      reached; otherwise it is [0, infinity].
      The lower bounds are set to 0 in the constructor and never change.
    */
    int num_cols = lm_graph.number_of_landmarks();
    for (int lm_id = 0; lm_id < num_cols; ++lm_id) {
        const LandmarkNode *lm = lm_graph.get_lm_for_index(lm_id);
        if (lm->get_status() == lm_reached) {
            lp_variables[lm_id].upper_bound = 0;
        } else {
            lp_variables[lm_id].upper_bound = lp_solver.get_infinity();
        }
    }

    /*
      Define the constraint matrix. The constraints are of the form
      cost(lm_i1) + cost(lm_i2) + ... + cost(lm_in) <= cost(o)
      where lm_i1 ... lm_in are the landmarks for which o is a
      relevant achiever. Hence, we add a triple (op, lm, 1.0)
      for each relevant achiever op of landmark lm, denoting that
      in the op-th row and lm-th column, the matrix has a 1.0 entry.
    */
    // Reuse previous constraint objects to save the effort of recreating them.
    for (lp::LPConstraint &constraint : lp_constraints) {
        constraint.clear();
    }
    for (int lm_id = 0; lm_id < num_cols; ++lm_id) {
        const LandmarkNode *lm = lm_graph.get_lm_for_index(lm_id);
        int lm_status = lm->get_status();
        if (lm_status != lm_reached) {
            const set<int> &achievers = get_achievers(lm_status, *lm);
            assert(!achievers.empty());
            for (int op_id : achievers) {
                assert(!get_operator_or_axiom(task_proxy, op_id).is_axiom());
                lp_constraints[op_id].insert(lm_id, 1.0);
            }
        }
    }

    /* Copy non-empty constraints and use those in the LP.
       This significantly speeds up the heuristic calculation. See issue443. */
    // TODO: do not copy the data here.
    non_empty_lp_constraints.clear();
    for (const lp::LPConstraint &constraint : lp_constraints) {
        if (!constraint.empty())
            non_empty_lp_constraints.push_back(constraint);
    }

    // Load the problem into the LP solver.
    lp_solver.load_problem(lp::LPObjectiveSense::MAXIMIZE,
                           lp_variables, non_empty_lp_constraints);

    // Solve the linear program.
    lp_solver.solve();

    assert(lp_solver.has_optimal_solution());
    double h = lp_solver.get_objective_value();

    return h;
}
}
