#include "landmark_cost_partitioning_algorithms.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "landmark_status_manager.h"
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
CostPartitioningAlgorithm::CostPartitioningAlgorithm(
    const vector<int> &operator_costs, const LandmarkGraph &graph)
    : lm_graph(graph), operator_costs(operator_costs) {
}

const set<int> &CostPartitioningAlgorithm::get_achievers(
    const Landmark &landmark, bool past) const {
    // Return relevant achievers of the landmark according to its status.
    if (past) {
        return landmark.possible_achievers;
    } else {
        return landmark.first_achievers;
    }
}


UniformCostPartitioningAlgorithm::UniformCostPartitioningAlgorithm(
    const vector<int> &operator_costs, const LandmarkGraph &graph,
    bool use_action_landmarks)
    : CostPartitioningAlgorithm(operator_costs, graph),
      use_action_landmarks(use_action_landmarks) {
}

double UniformCostPartitioningAlgorithm::get_cost_partitioned_heuristic_value(
    const LandmarkStatusManager &lm_status_manager,
    const State &ancestor_state) {
    vector<int> achieved_lms_by_op(operator_costs.size(), 0);
    vector<bool> action_landmarks(operator_costs.size(), false);

    const LandmarkGraph::Nodes &nodes = lm_graph.get_nodes();
    ConstBitsetView past =
        lm_status_manager.get_past_landmarks(ancestor_state);
    ConstBitsetView future =
        lm_status_manager.get_future_landmarks(ancestor_state);

    double h = 0;

    /* First pass:
       compute which op achieves how many landmarks. Along the way,
       mark action landmarks and add their cost to h. */
    for (auto &node : nodes) {
        int id = node->get_id();
        if (future.test(id)) {
            const set<int> &achievers =
                get_achievers(node->get_landmark(), past.test(id));
            if (achievers.empty())
                return numeric_limits<double>::max();
            if (use_action_landmarks && achievers.size() == 1) {
                // We have found an action landmark for this state.
                int op_id = *achievers.begin();
                if (!action_landmarks[op_id]) {
                    action_landmarks[op_id] = true;
                    assert(utils::in_bounds(op_id, operator_costs));
                    h += operator_costs[op_id];
                }
            } else {
                for (int op_id : achievers) {
                    assert(utils::in_bounds(op_id, achieved_lms_by_op));
                    ++achieved_lms_by_op[op_id];
                }
            }
        }
    }

    /* TODO: Replace with Landmarks (to do so, we need some way to access the
        status of a Landmark without access to the ID, which is part of
        LandmarkNode). */
    vector<LandmarkNode *> relevant_lms;

    /* Second pass:
       remove landmarks from consideration that are covered by
       an action landmark; decrease the counters accordingly
       so that no unnecessary cost is assigned to these landmarks. */
    for (auto &node : nodes) {
        int id = node->get_id();
        if (future.test(id)) {
            const set<int> &achievers =
                get_achievers(node->get_landmark(), past.test(id));
            bool covered_by_action_lm = false;
            for (int op_id : achievers) {
                assert(utils::in_bounds(op_id, action_landmarks));
                if (action_landmarks[op_id]) {
                    covered_by_action_lm = true;
                    break;
                }
            }
            if (covered_by_action_lm) {
                for (int op_id : achievers) {
                    assert(utils::in_bounds(op_id, achieved_lms_by_op));
                    --achieved_lms_by_op[op_id];
                }
            } else {
                relevant_lms.push_back(node.get());
            }
        }
    }

    /* Third pass:
       count shared costs for the remaining landmarks. */
    for (const LandmarkNode *node : relevant_lms) {
        // TODO: Iterate over Landmarks instead of LandmarkNodes
        int id = node->get_id();
        assert(future.test(id));
        const set<int> &achievers =
            get_achievers(node->get_landmark(), past.test(id));
        double min_cost = numeric_limits<double>::max();
        for (int op_id : achievers) {
            assert(utils::in_bounds(op_id, achieved_lms_by_op));
            int num_achieved = achieved_lms_by_op[op_id];
            assert(num_achieved >= 1);
            assert(utils::in_bounds(op_id, operator_costs));
            double partitioned_cost =
                static_cast<double>(operator_costs[op_id]) / num_achieved;
            min_cost = min(min_cost, partitioned_cost);
        }
        h += min_cost;
    }

    return h;
}


OptimalCostPartitioningAlgorithm::OptimalCostPartitioningAlgorithm(
    const vector<int> &operator_costs, const LandmarkGraph &graph,
    lp::LPSolverType solver_type)
    : CostPartitioningAlgorithm(operator_costs, graph),
      lp_solver(solver_type),
      lp(build_initial_lp()) {
}

lp::LinearProgram OptimalCostPartitioningAlgorithm::build_initial_lp() {
    /* The LP has one variable (column) per landmark and one
       inequality (row) per operator. */
    int num_cols = lm_graph.get_num_landmarks();
    int num_rows = operator_costs.size();

    named_vector::NamedVector<lp::LPVariable> lp_variables;

    /* We want to maximize 1 * cost(lm_1) + ... + 1 * cost(lm_n),
       so the coefficients are all 1.
       Variable bounds are state-dependent; we initialize the range to {0}. */
    lp_variables.resize(num_cols, lp::LPVariable(0.0, 0.0, 1.0));

    /* Set up lower bounds and upper bounds for the inequalities.
       These simply say that the operator's total cost must fall
       between 0 and the real operator cost. */
    lp_constraints.resize(num_rows, lp::LPConstraint(0.0, 0.0));
    for (size_t op_id = 0; op_id < operator_costs.size(); ++op_id) {
        lp_constraints[op_id].set_lower_bound(0);
        lp_constraints[op_id].set_upper_bound(operator_costs[op_id]);
    }

    /* Coefficients of constraints will be updated and recreated in each state.
       We ignore them for the initial LP. */
    return lp::LinearProgram(lp::LPObjectiveSense::MAXIMIZE, move(lp_variables),
                             {}, lp_solver.get_infinity());
}

double OptimalCostPartitioningAlgorithm::get_cost_partitioned_heuristic_value(
    const LandmarkStatusManager &lm_status_manager,
    const State &ancestor_state) {
    /* TODO: We could also do the same thing with action landmarks we
             do in the uniform cost partitioning case. */


    ConstBitsetView past =
        lm_status_manager.get_past_landmarks(ancestor_state);
    ConstBitsetView future =
        lm_status_manager.get_future_landmarks(ancestor_state);
    /*
      Set up LP variable bounds for the landmarks.
      The range of cost(lm_1) is {0} if the landmark is already
      reached; otherwise it is [0, infinity].
      The lower bounds are set to 0 in the constructor and never change.
    */
    int num_cols = lm_graph.get_num_landmarks();
    for (int lm_id = 0; lm_id < num_cols; ++lm_id) {
        if (future.test(lm_id)) {
            lp.get_variables()[lm_id].upper_bound = lp_solver.get_infinity();
        } else {
            lp.get_variables()[lm_id].upper_bound = 0;
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
        const Landmark &landmark = lm_graph.get_node(lm_id)->get_landmark();
        if (future.test(lm_id)) {
            const set<int> &achievers =
                get_achievers(landmark, past.test(lm_id));
            if (achievers.empty())
                return numeric_limits<double>::max();
            for (int op_id : achievers) {
                assert(utils::in_bounds(op_id, lp_constraints));
                lp_constraints[op_id].insert(lm_id, 1.0);
            }
        }
    }

    /* Copy non-empty constraints and use those in the LP.
       This significantly speeds up the heuristic calculation. See issue443. */
    // TODO: do not copy the data here.
    lp.get_constraints().clear();
    for (const lp::LPConstraint &constraint : lp_constraints) {
        if (!constraint.empty())
            lp.get_constraints().push_back(constraint);
    }

    // Load the problem into the LP solver.
    lp_solver.load_problem(lp);

    // Solve the linear program.
    lp_solver.solve();

    assert(lp_solver.has_optimal_solution());
    double h = lp_solver.get_objective_value();

    return h;
}
}
