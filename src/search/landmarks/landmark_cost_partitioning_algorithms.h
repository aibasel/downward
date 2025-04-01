#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_ALGORITHMS_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_ALGORITHMS_H

#include "../task_proxy.h"

#include "../lp/lp_solver.h"

#include <unordered_set>
#include <vector>

#include "../per_state_bitset.h"

class ConstBitsetView;
class OperatorsProxy;

namespace landmarks {
class Landmark;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class CostPartitioningAlgorithm {
protected:
    const LandmarkGraph &landmark_graph;
    const std::vector<int> operator_costs;
public:
    CostPartitioningAlgorithm(const std::vector<int> &operator_costs,
                              const LandmarkGraph &graph);
    virtual ~CostPartitioningAlgorithm() = default;

    virtual double get_cost_partitioned_heuristic_value(
        const LandmarkStatusManager &lm_status_manager,
        const State &ancestor_state) = 0;
};

class UniformCostPartitioningAlgorithm : public CostPartitioningAlgorithm {
    bool use_action_landmarks;

    /*
      TODO: We are aware that the following three function names are not
       meaningful descriptions of what they do. We introduced these functions
       in issue992 in an attempt to make the code more readable (e.g., by
       breaking apart long functions) without changing its behavior. Since we
       would like to implement computing the cost partitioning differently, and
       because these functions do not have just one simple purpose, we did not
       bother trying to find descriptive function names at this time.
    */
    double first_pass(
        std::vector<int> &landmarks_achieved_by_operator,
        std::vector<bool> &action_landmarks,
        ConstBitsetView &past, ConstBitsetView &future);
    std::vector<const LandmarkNode *> second_pass(
        std::vector<int> &landmarks_achieved_by_operator,
        const std::vector<bool> &action_landmarks, ConstBitsetView &past,
        ConstBitsetView &future);
    double third_pass(
        const std::vector<const LandmarkNode *> &uncovered_landmarks,
        const std::vector<int> &landmarks_achieved_by_operator,
        ConstBitsetView &past, ConstBitsetView &future);
public:
    UniformCostPartitioningAlgorithm(const std::vector<int> &operator_costs,
                                     const LandmarkGraph &graph,
                                     bool use_action_landmarks);

    virtual double get_cost_partitioned_heuristic_value(
        const LandmarkStatusManager &landmark_status_manager,
        const State &ancestor_state) override;
};

class OptimalCostPartitioningAlgorithm : public CostPartitioningAlgorithm {
    lp::LPSolver lp_solver;
    /* We keep an additional copy of the constraints around to avoid
       some effort with recreating the vector (see issue443). */
    std::vector<lp::LPConstraint> lp_constraints;
    /*
      We keep the vectors for LP variables and constraints around instead of
      recreating them for every state. The actual constraints have to be
      recreated because the coefficient matrix of the LP changes from state to
      state. Reusing the vectors still saves some dynamic allocation overhead.
     */
    lp::LinearProgram lp;

    lp::LinearProgram build_initial_lp();
    void set_lp_bounds(ConstBitsetView &future, int num_cols);
    bool define_constraint_matrix(
        ConstBitsetView &past, ConstBitsetView &future, int num_cols);
public:
    OptimalCostPartitioningAlgorithm(const std::vector<int> &operator_costs,
                                     const LandmarkGraph &graph,
                                     lp::LPSolverType solver_type);

    virtual double get_cost_partitioned_heuristic_value(
        const LandmarkStatusManager &landmark_status_manager,
        const State &ancestor_state) override;
};
}

#endif
