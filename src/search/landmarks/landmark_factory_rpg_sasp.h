#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
class LandmarkFactoryRpgSasp : public LandmarkFactory {
    std::list<LandmarkNode *> open_landmarks;
    std::vector<std::vector<int>> disjunction_classes;

    // dtg_successors[var_id][val] contains all successor values of val in the
    // domain transition graph for the variable
    std::vector<std::vector<std::unordered_set<int>>> dtg_successors;

    void build_dtg_successors(const TaskProxy &task_proxy);
    void add_dtg_successor(int var_id, int pre, int post);
    void find_forward_orders(const VariablesProxy &variables,
                             const std::vector<std::vector<int>> &lvl_var,
                             LandmarkNode *lmp);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(const TaskProxy &task_proxy,
                                         const LandmarkNode *lmp,
                                         const OperatorProxy &op,
                                         std::unordered_map<int, int> &result) const;
    void compute_shared_preconditions(const TaskProxy &task_proxy,
                                      std::unordered_map<int, int> &shared_pre,
                                      std::vector<std::vector<int>> &lvl_var,
                                      LandmarkNode *bp);
    void compute_disjunctive_preconditions(
        const TaskProxy &task_proxy,
        std::vector<std::set<FactPair>> &disjunctive_pre,
        std::vector<std::vector<int>> &lvl_var, LandmarkNode *bp);

    int min_cost_for_landmark(const TaskProxy &task_proxy,
                              LandmarkNode *bp,
                              std::vector<std::vector<int>> &lvl_var);
    virtual void generate_landmarks(const std::shared_ptr<AbstractTask> &task,
                                    Exploration &exploration) override;
    void found_simple_lm_and_order(const FactPair &a, LandmarkNode &b,
                                   EdgeType t);
    void found_disj_lm_and_order(const TaskProxy &task_proxy,
                                 const std::set<FactPair> &a,
                                 LandmarkNode &b,
                                 EdgeType t);
    void approximate_lookahead_orders(const TaskProxy &task_proxy,
                                      const std::vector<std::vector<int>> &lvl_var,
                                      LandmarkNode *lmp);
    bool domain_connectivity(const State &initial_state,
                             const FactPair &landmark,
                             const std::unordered_set<int> &exclude);

    void build_disjunction_classes(const TaskProxy &task_proxy);
public:
    explicit LandmarkFactoryRpgSasp(const options::Options &opts);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
