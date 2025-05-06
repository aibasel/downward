#include "landmark_cost_partitioning_algorithms.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "landmark_status_manager.h"

#include "../utils/collections.h"
#include "../utils/language.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

using namespace std;

namespace landmarks {
CostPartitioningAlgorithm::CostPartitioningAlgorithm(
    const vector<int> &operator_costs, const LandmarkGraph &graph)
    : landmark_graph(graph), operator_costs(operator_costs) {
}

static const unordered_set<int> &get_achievers(
    const Landmark &landmark, const bool past) {
    // Return relevant achievers of the landmark according to its status.
    return past ? landmark.possible_achievers : landmark.first_achievers;
}

UniformCostPartitioningAlgorithm::UniformCostPartitioningAlgorithm(
    const vector<int> &operator_costs, const LandmarkGraph &graph,
    const bool use_action_landmarks)
    : CostPartitioningAlgorithm(operator_costs, graph),
      use_action_landmarks(use_action_landmarks) {
}

/* Compute which operator achieves how many landmarks. Along the way, mark
   action landmarks and sum up their costs. */
double UniformCostPartitioningAlgorithm::first_pass(
    vector<int> &landmarks_achieved_by_operator,
    vector<bool> &action_landmarks,
    ConstBitsetView &past, ConstBitsetView &future) {
    double action_landmarks_cost = 0;
    for (const auto &node : landmark_graph) {
        int id = node->get_id();
        if (future.test(id)) {
            const unordered_set<int> &achievers =
                get_achievers(node->get_landmark(), past.test(id));
            if (achievers.empty()) {
                return numeric_limits<double>::max();
            }
            if (use_action_landmarks && achievers.size() == 1) {
                // We have found an action landmark for this state.
                int op_id = *achievers.begin();
                if (!action_landmarks[op_id]) {
                    action_landmarks[op_id] = true;
                    assert(utils::in_bounds(op_id, operator_costs));
                    action_landmarks_cost += operator_costs[op_id];
                }
            } else {
                for (int op_id : achievers) {
                    assert(utils::in_bounds(op_id, landmarks_achieved_by_operator));
                    ++landmarks_achieved_by_operator[op_id];
                }
            }
        }
    }
    return action_landmarks_cost;
}

/*
  Collect all landmarks that are not covered by action landmarks. For all
  landmarks that are covered, reduce the number of landmarks achieved by their
  achievers to strengthen the cost partitioning.
*/
vector<const LandmarkNode *> UniformCostPartitioningAlgorithm::second_pass(
    vector<int> &landmarks_achieved_by_operator,
    const vector<bool> &action_landmarks,
    ConstBitsetView &past, ConstBitsetView &future) {
    vector<const LandmarkNode *> uncovered_landmarks;
    for (const auto &node : landmark_graph) {
        int id = node->get_id();
        if (future.test(id)) {
            const unordered_set<int> &achievers =
                get_achievers(node->get_landmark(), past.test(id));
            bool covered_by_action_landmark = false;
            for (int op_id : achievers) {
                assert(utils::in_bounds(op_id, action_landmarks));
                if (action_landmarks[op_id]) {
                    covered_by_action_landmark = true;
                    break;
                }
            }
            if (covered_by_action_landmark) {
                for (int op_id : achievers) {
                    assert(utils::in_bounds(
                               op_id, landmarks_achieved_by_operator));
                    --landmarks_achieved_by_operator[op_id];
                }
            } else {
                uncovered_landmarks.push_back(node.get());
            }
        }
    }
    return uncovered_landmarks;
}

// Compute the cost partitioning.
double UniformCostPartitioningAlgorithm::third_pass(
    const vector<const LandmarkNode *> &uncovered_landmarks,
    const vector<int> &landmarks_achieved_by_operator,
    ConstBitsetView &past, ConstBitsetView &future) {
    double cost = 0;
    for (const LandmarkNode *node : uncovered_landmarks) {
        // TODO: Iterate over Landmarks instead of LandmarkNodes.
        int id = node->get_id();
        assert(future.test(id));
        utils::unused_variable(future);
        const unordered_set<int> &achievers =
            get_achievers(node->get_landmark(), past.test(id));
        double min_cost = numeric_limits<double>::max();
        for (int op_id : achievers) {
            assert(utils::in_bounds(op_id, landmarks_achieved_by_operator));
            int num_achieved = landmarks_achieved_by_operator[op_id];
            assert(num_achieved >= 1);
            assert(utils::in_bounds(op_id, operator_costs));
            double partitioned_cost =
                static_cast<double>(operator_costs[op_id]) / num_achieved;
            min_cost = min(min_cost, partitioned_cost);
        }
        cost += min_cost;
    }
    return cost;
}

double UniformCostPartitioningAlgorithm::get_cost_partitioned_heuristic_value(
    const LandmarkStatusManager &landmark_status_manager,
    const State &ancestor_state) {
    vector<int> landmarks_achieved_by_operator(operator_costs.size(), 0);
    vector<bool> action_landmarks(operator_costs.size(), false);

    ConstBitsetView past =
        landmark_status_manager.get_past_landmarks(ancestor_state);
    ConstBitsetView future =
        landmark_status_manager.get_future_landmarks(ancestor_state);

    const double cost_of_action_landmarks = first_pass(
        landmarks_achieved_by_operator, action_landmarks, past, future);
    if (cost_of_action_landmarks == numeric_limits<double>::max()) {
        return cost_of_action_landmarks;
    }

    /*
      TODO: Use landmarks instead of landmark nodes. To do so, we need
       some way to access the status of a Landmark without access to the
       ID which is part of landmark node.
    */
    const vector<const LandmarkNode *> uncovered_landmarks = second_pass(
        landmarks_achieved_by_operator, action_landmarks, past, future);

    const double cost_partitioning_cost = third_pass(
        uncovered_landmarks, landmarks_achieved_by_operator, past, future);

    return cost_of_action_landmarks + cost_partitioning_cost;
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
    const int num_cols = landmark_graph.get_num_landmarks();
    const int num_rows = operator_costs.size();

    named_vector::NamedVector<lp::LPVariable> lp_variables;

    /*
      We want to maximize 1 * cost(lm_1) + ... + 1 * cost(lm_n), so the
      coefficients are all 1.
      Variable bounds are state-dependent; we initialize the range to {0}.
    */
    lp_variables.resize(num_cols, lp::LPVariable(0.0, 0.0, 1.0));

    /*
      Set up lower bounds and upper bounds for the inequalities. These simply
      say that the operator's total cost must fall between 0 and the real
      operator cost.
    */
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

/*
  Set up LP variable bounds for the landmarks. The range of cost(lm_1) is {0} if
  the landmark is already reached; otherwise it is [0, infinity]. The lower
  bounds are set to 0 in the constructor and never change.
*/
void OptimalCostPartitioningAlgorithm::set_lp_bounds(
    ConstBitsetView &future, const int num_cols) {
    for (int id = 0; id < num_cols; ++id) {
        if (future.test(id)) {
            lp.get_variables()[id].upper_bound = lp_solver.get_infinity();
        } else {
            lp.get_variables()[id].upper_bound = 0;
        }
    }
}

/*
  Define the constraint matrix. The constraints are of the form
  cost(lm_i1) + cost(lm_i2) + ... + cost(lm_in) <= cost(o)
  where lm_i1 ... lm_in are the landmarks for which o is a relevant achiever.
  Hence, we add a triple (op, lm, 1.0) for each relevant achiever op of
  landmark lm, denoting that in the op-th row and lm-th column, the matrix has
  a 1.0 entry.
  Returns true if the current state is a dead-end.
*/
bool OptimalCostPartitioningAlgorithm::define_constraint_matrix(
    ConstBitsetView &past, ConstBitsetView &future, const int num_cols) {
    // Reuse previous constraint objects to save the effort of recreating them.
    for (lp::LPConstraint &constraint : lp_constraints) {
        constraint.clear();
    }
    for (int id = 0; id < num_cols; ++id) {
        const Landmark &landmark = landmark_graph.get_node(id)->get_landmark();
        if (future.test(id)) {
            const unordered_set<int> &achievers =
                get_achievers(landmark, past.test(id));
            /*
              TODO: We could deal with things more uniformly by just adding a
               constraint with no variables because there are no achievers
               (instead of returning here), which would then be detected as an
               unsolvable constraint by the LP solver. However, as of now this
               does not work because `get_cost_partitioned_heuristic_value` only
               adds non-empty constraints to the LP. We should implement this
               differently, which requires a solution that does not reuse
               constraints from the previous iteration as it does now.
            */
            if (achievers.empty()) {
                return true;
            }
            for (int op_id : achievers) {
                assert(utils::in_bounds(op_id, lp_constraints));
                lp_constraints[op_id].insert(id, 1.0);
            }
        }
    }
    return false;
}


double OptimalCostPartitioningAlgorithm::get_cost_partitioned_heuristic_value(
    const LandmarkStatusManager &landmark_status_manager,
    const State &ancestor_state) {
    /* TODO: We could also do the same thing with action landmarks we do in the
        uniform cost partitioning case. */

    ConstBitsetView past =
        landmark_status_manager.get_past_landmarks(ancestor_state);
    ConstBitsetView future =
        landmark_status_manager.get_future_landmarks(ancestor_state);

    const int num_cols = landmark_graph.get_num_landmarks();
    set_lp_bounds(future, num_cols);
    const bool dead_end = define_constraint_matrix(past, future, num_cols);
    if (dead_end) {
        return numeric_limits<double>::max();
    }

    /* Copy non-empty constraints and use those in the LP.
       This significantly speeds up the heuristic calculation. See issue443. */
    // TODO: Do not copy the data here.
    lp.get_constraints().clear();
    for (const lp::LPConstraint &constraint : lp_constraints) {
        if (!constraint.empty()) {
            lp.get_constraints().push_back(constraint);
        }
    }

    lp_solver.load_problem(lp);
    lp_solver.solve();

    assert(lp_solver.has_optimal_solution());
    return lp_solver.get_objective_value();
}
}
