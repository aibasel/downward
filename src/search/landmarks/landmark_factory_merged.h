#ifndef LANDMARKS_LANDMARK_FACTORY_MERGED_H
#define LANDMARKS_LANDMARK_FACTORY_MERGED_H

#include "landmark_factory.h"

#include <vector>

namespace landmarks {
class Exploration;

class LandmarkFactoryMerged : public LandmarkFactory {
    std::vector<std::shared_ptr<LandmarkFactory>> lm_factories;

    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task) override;
    void generate(const TaskProxy &task_proxy);
    void calc_achievers(const TaskProxy &task_proxy);
    bool relaxed_task_solvable(const TaskProxy &task_proxy, Exploration &exploration,
                               std::vector<std::vector<int>> &lvl_var,
                               std::vector<utils::HashMap<FactPair, int>> &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;
    bool achieves_non_conditional(const OperatorProxy &o,
                                  const LandmarkNode *lmp) const;
    void add_operator_and_propositions_to_list(
        const OperatorProxy &op, std::vector<utils::HashMap<FactPair, int>> &lvl_op) const;
    LandmarkNode *get_matching_landmark(const LandmarkNode &lm) const;
public:
    explicit LandmarkFactoryMerged(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
