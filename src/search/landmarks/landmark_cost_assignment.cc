#include "landmark_cost_assignment.h"

#include "landmark_graph.h"

#include "../linear_program.h"
#include "../utilities.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <sys/times.h>


using namespace std;


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


/* Uniform cost partioning */
LandmarkUniformSharedCostAssignment::LandmarkUniformSharedCostAssignment(
    LandmarkGraph &graph, bool use_action_landmarks_, OperatorCost cost_type_)
    : LandmarkCostAssignment(graph, cost_type_), use_action_landmarks(use_action_landmarks_) {
}


LandmarkUniformSharedCostAssignment::~LandmarkUniformSharedCostAssignment() {
}


double LandmarkUniformSharedCostAssignment::cost_sharing_h_value() {
    vector<int> achieved_lms_by_op(g_operators.size(), 0);
    vector<bool> action_landmarks(g_operators.size(), false);

    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
    set<LandmarkNode *>::iterator node_it;

    double h = 0;

    // First pass: compute which op achieves how many landmarks.
    // Along the way, mark action landmarks and add their cost to h.
    for (node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
        LandmarkNode &node = **node_it;
        int lmn_status = node.get_status();
        if (lmn_status != lm_reached) {
            const set<int> &achievers = get_achievers(lmn_status, node);
            assert(!achievers.empty());
            if (use_action_landmarks && achievers.size() == 1) {
                // We have found an action landmark for this state.
                int op_id = *achievers.begin();
                assert(in_bounds(op_id, g_operators));
                if (!action_landmarks[op_id]) {
                    action_landmarks[op_id] = true;
                    const GlobalOperator &op = lm_graph.get_operator_for_lookup_index(
                        op_id);
                    h += get_adjusted_action_cost(op, cost_type);
                }
            } else {
                set<int>::const_iterator ach_it;
                for (ach_it = achievers.begin(); ach_it != achievers.end();
                     ++ach_it) {
                    int op_id = *ach_it;
                    assert(in_bounds(op_id, g_operators));
                    ++achieved_lms_by_op[op_id];
                }
            }
        }
    }

    vector<LandmarkNode *> relevant_lms;

    // Second pass: Remove landmarks from consideration that are
    // covered by an action landmark; decrease the counters accordingly
    // so that no unnecessary cost is assigned to these landmarks.
    for (node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
        LandmarkNode &node = **node_it;
        int lmn_status = node.get_status();
        if (lmn_status != lm_reached) {
            const set<int> &achievers = get_achievers(lmn_status, node);
            set<int>::const_iterator ach_it;
            bool covered_by_action_lm = false;
            for (ach_it = achievers.begin(); ach_it != achievers.end();
                 ++ach_it) {
                int op_id = *ach_it;
                assert(in_bounds(op_id, g_operators));
                if (action_landmarks[op_id]) {
                    covered_by_action_lm = true;
                    break;
                }
            }
            if (covered_by_action_lm) {
                for (ach_it = achievers.begin(); ach_it != achievers.end();
                     ++ach_it) {
                    int op_id = *ach_it;
                    assert(in_bounds(op_id, g_operators));
                    --achieved_lms_by_op[op_id];
                }
            } else {
                relevant_lms.push_back(&node);
            }
        }
    }

    // Third pass: Count shared costs for the remaining landmarks.
    for (size_t i = 0; i < relevant_lms.size(); ++i) {
        LandmarkNode &node = *relevant_lms[i];
        int lmn_status = node.get_status();
        const set<int> &achievers = get_achievers(lmn_status, node);
        set<int>::const_iterator ach_it;
        double min_cost = numeric_limits<double>::max();
        for (ach_it = achievers.begin(); ach_it != achievers.end();
             ++ach_it) {
            int op_id = *ach_it;
            assert(in_bounds(op_id, g_operators));
            const GlobalOperator &op = lm_graph.get_operator_for_lookup_index(
                op_id);
            int num_achieved = achieved_lms_by_op[op_id];
            assert(num_achieved >= 1);
            double shared_cost = double(get_adjusted_action_cost(op, cost_type)) / num_achieved;
            min_cost = min(min_cost, shared_cost);
        }
        h += min_cost;
    }

    return h;
}

LandmarkEfficientOptimalSharedCostAssignment::LandmarkEfficientOptimalSharedCostAssignment(
    LandmarkGraph &graph, OperatorCost cost_type, LPSolverType solver_type_)
    : LandmarkCostAssignment(graph, cost_type),
      solver_type(solver_type_) {
}

LandmarkEfficientOptimalSharedCostAssignment::~LandmarkEfficientOptimalSharedCostAssignment() {
}

double LandmarkEfficientOptimalSharedCostAssignment::cost_sharing_h_value() {
    // TODO: This could probably be speeded up by taking some of
    //       the relevant dynamic memory management out of the
    //       method, using loadProblem instead of assignProblem.
    // TODO: We could also do the same thing with action landmarks we
    //       do in the uniform cost partitioning case.

    struct tms start, end_build, end_solve, end_all;
    times(&start);

    // The LP has one variable (column) per landmark and one
    // inequality (row) per operator.
    int num_cols = lm_graph.number_of_landmarks();
    int num_rows = g_operators.size();

    // Set up lower bounds, upper bounds and objective function
    // coefficients for the landmarks.
    // We want to maximize 1 * cost(lm_1) + ... + 1 * cost(lm_n),
    // so the coefficients are all 1.
    // The range of cost(lm_1) is {0} if the landmark is already
    // reached; otherwise it is [0, infinity].
    vector<LPVariable> variables(num_cols, LPVariable(0.0, 0.0, 1.0));
    for (int lm_id = 0; lm_id < num_cols; ++lm_id) {
        const LandmarkNode *lm = lm_graph.get_lm_for_index(lm_id);
        if (lm->get_status() != lm_reached) {
            variables[lm_id].upper_bound = numeric_limits<double>::infinity();
        }
    }

    // Set up lower bounds and upper bounds for the inequalities.
    // These simply say that the operator's total cost must fall
    // between 0 and the real operator cost.
    vector<LPConstraint> constraints(num_rows);
    for (size_t op_id = 0; op_id < g_operators.size(); ++op_id) {
        const GlobalOperator &op = g_operators[op_id];
        constraints[op_id].lower_bound = 0;
        constraints[op_id].upper_bound = get_adjusted_action_cost(op, cost_type);
    }

    // Define the constraint matrix. The constraints are of the form
    // cost(lm_i1) + cost(lm_i2) + ... + cost(lm_in) <= cost(o)
    // where lm_i1 ... lm_in are the landmarks for which o is a
    // relevant achiever. Hence, we add a triple (op, lm, 1.0)
    // for each relevant achiever op of landmark lm, denoting that
    // in the op-th row and lm-th column, the matrix has a 1.0 entry.
    for (int lm_id = 0; lm_id < num_cols; ++lm_id) {
        const LandmarkNode *lm = lm_graph.get_lm_for_index(lm_id);
        int lm_status = lm->get_status();
        if (lm_status != lm_reached) {
            const set<int> &achievers = get_achievers(lm_status, *lm);
            assert(!achievers.empty());
            set<int>::const_iterator ach_it;
            for (ach_it = achievers.begin(); ach_it != achievers.end();
                 ++ach_it) {
                int op_id = *ach_it;
                assert(in_bounds(op_id, g_operators));
                constraints[op_id].insert(lm_id, 1.0);
            }
        }
    }
    LP lp(solver_type, variables, constraints, LPObjectiveSense::MAXIMIZE);
    times(&end_build);

    // Solve the linear program.
    lp.solve();
    times(&end_solve);

    assert(lp.has_optimal_solution());
    double h = lp.get_objective_value();

    // We might call si->reset() here, but this makes the overall
    // code a bit slower in small tests, presumably due to dynamic
    // memory managment overhead in the LP library. So we don't
    // call it. This LP will be cleaned up once the next one is
    // constructed.

    times(&end_all);
    /*
    int total_ms = (end_all.tms_utime - start.tms_utime) * 10;
    int build_ms = (end_build.tms_utime - start.tms_utime) * 10;
    int solve_ms = (end_solve.tms_utime - end_build.tms_utime) * 10;

    cout << "Build: " << build_ms << " , Solve: " << solve_ms
         << " , Total: " << total_ms << endl;
    */
    return h;
}
