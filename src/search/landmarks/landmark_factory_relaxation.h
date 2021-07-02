#ifndef LANDMARKS_LANDMARK_FACTORY_RELAXATION_H
#define LANDMARKS_LANDMARK_FACTORY_RELAXATION_H

#include "landmark_factory.h"

namespace landmarks {
class Exploration;

class LandmarkFactoryRelaxation : public LandmarkFactory {
protected:
    LandmarkFactoryRelaxation() = default;

    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               bool level_out,
                               const Landmark &exclude,
                               bool compute_lvl_op = false) const;
    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               std::vector<std::vector<int>> &lvl_var,
                               std::vector<utils::HashMap<FactPair, int>> &lvl_op,
                               bool level_out,
                               const Landmark &exclude,
                               bool compute_lvl_op = false) const;

private:
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) = 0;
    void postprocess(const TaskProxy &task_proxy, Exploration &exploration);

    void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration);
    bool achieves_non_conditional(const OperatorProxy &o,
                                  const Landmark &landmark) const;
    void add_operator_and_propositions_to_list(
        const OperatorProxy &op, std::vector<utils::HashMap<FactPair, int>> &lvl_op) const;

protected:
    /*
      The method discard_noncausal_landmarks assumes the graph has no conjunctive
      landmarks, and will not process conjunctive landmarks correctly.
    */
    void discard_noncausal_landmarks(const TaskProxy &task_proxy,
                                     Exploration &exploration);
    bool is_causal_landmark(const TaskProxy &task_proxy,
                            Exploration &exploration,
                            const Landmark &landmark) const;
};
}

#endif
