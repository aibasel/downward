#ifndef LANDMARKS_LANDMARK_COUNT_HEURISTIC_H
#define LANDMARKS_LANDMARK_COUNT_HEURISTIC_H

#include "../heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkCostAssignment;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkCountHeuristic : public Heuristic {
    std::shared_ptr<LandmarkGraph> lgraph;
    const bool use_preferred_operators;
    const bool conditional_effects_supported;
    const bool admissible;
    const bool dead_ends_reliable;

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<LandmarkCostAssignment> lm_cost_assignment;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    /*
      TODO: The following vectors are only used in the non-admissible case. This
       is bad design, but given this is already the case for
       *lm_cost_assignment* above, this is just another indicator to change this
       in the future.
    */
    std::vector<int> min_first_achiever_costs;
    std::vector<int> min_possible_achiever_costs;

    int get_heuristic_value(const State &ancestor_state);

    bool landmark_is_interesting(
        const State &state, LandmarkNode &lm_node) const;
    void generate_preferred_operators(const State &state);

    int get_min_cost_of_achievers(const std::set<int> &achievers,
                                  const TaskProxy &task_proxy);
    void compute_landmark_costs();
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkCountHeuristic(const options::Options &opts);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
