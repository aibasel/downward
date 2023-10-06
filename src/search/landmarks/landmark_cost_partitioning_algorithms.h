#ifndef LANDMARKS_LANDMARK_COST_PARTITIONING_ALGORITHMS_H
#define LANDMARKS_LANDMARK_COST_PARTITIONING_ALGORITHMS_H

#include "../task_proxy.h"

#include "../lp/lp_solver.h"

#include <unordered_set>
#include <vector>

class OperatorsProxy;

namespace landmarks {
class Landmark;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class CostPartitioningAlgorithm {
protected:
    const LandmarkGraph &lm_graph;
    const std::vector<int> operator_costs;

    const std::unordered_set<int> &get_achievers(
        const Landmark &landmark, bool past) const;
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
public:
    UniformCostPartitioningAlgorithm(const std::vector<int> &operator_costs,
                                     const LandmarkGraph &graph,
                                     bool use_action_landmarks);

    virtual double get_cost_partitioned_heuristic_value(
        const LandmarkStatusManager &lm_status_manager,
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
public:
    OptimalCostPartitioningAlgorithm(const std::vector<int> &operator_costs,
                                     const LandmarkGraph &graph,
                                     lp::LPSolverType solver_type);

    virtual double get_cost_partitioned_heuristic_value(
        const LandmarkStatusManager &lm_status_manager,
        const State &ancestor_state) override;
};
}

#endif
