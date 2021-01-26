#ifndef LANDMARKS_LANDMARK_FACTORY_RELAXATION_H
#define LANDMARKS_LANDMARK_FACTORY_RELAXATION_H

#include "landmark_factory.h"

namespace landmarks {
class Exploration;

class LandmarkFactoryRelaxation : public LandmarkFactory {
protected:
    explicit LandmarkFactoryRelaxation(const options::Options &opts)
        : LandmarkFactory(opts) {}

    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               std::vector<std::vector<int>> &lvl_var,
                               std::vector<utils::HashMap<FactPair, int>> &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;

private:
    void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;

    virtual void generate_relaxed_landmarks(const std::shared_ptr<AbstractTask> &task,
                                            Exploration &exploration) = 0;
    void generate(const TaskProxy &task_proxy, Exploration &exploration);

    // TODO: this is duplicated here and in LandmarkFactoryHM
    void discard_noncausal_landmarks(const TaskProxy &task_proxy,
                                     Exploration &exploration);
    // TODO: this is duplicated here and in LandmarkFactoryHM
    bool is_causal_landmark(const TaskProxy &task_proxy,
                            Exploration &exploration,
                            const LandmarkNode &landmark) const;

    void calc_achievers(const TaskProxy &task_proxy, Exploration &exploration);
    bool achieves_non_conditional(const OperatorProxy &o,
                                  const LandmarkNode *lmp) const;
    void add_operator_and_propositions_to_list(
        const OperatorProxy &op, std::vector<utils::HashMap<FactPair, int>> &lvl_op) const;
};
}

#endif
