#ifndef LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H
#define LANDMARKS_LANDMARK_FACTORY_RPG_SASP_H

#include "landmark_factory_relaxation.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
class LandmarkFactoryRpgSasp : public LandmarkFactoryRelaxation {
    const bool disjunctive_landmarks;
    const bool use_orders;
    std::list<LandmarkNode *> open_landmarks;
    std::vector<std::vector<int>> disjunction_classes;

    std::unordered_map<LandmarkNode *, utils::HashSet<FactPair>> forward_orders;

    // dtg_successors[var_id][val] contains all successor values of val in the
    // domain transition graph for the variable
    std::vector<std::vector<std::unordered_set<int>>> dtg_successors;

    void build_dtg_successors(const TaskProxy &task_proxy);
    void add_dtg_successor(int var_id, int pre, int post);
    void find_forward_orders(const VariablesProxy &variables,
                             const std::vector<std::vector<bool>> &reached,
                             LandmarkNode *lm_node);
    void add_lm_forward_orders();

    void get_greedy_preconditions_for_lm(
        const TaskProxy &task_proxy, const Landmark &landmark,
        const OperatorProxy &op,
        std::unordered_map<int, int> &result) const;
    void compute_shared_preconditions(
        const TaskProxy &task_proxy,
        std::unordered_map<int, int> &shared_pre,
        std::vector<std::vector<bool>> &reached, const Landmark &landmark);
    void compute_disjunctive_preconditions(
        const TaskProxy &task_proxy,
        std::vector<std::set<FactPair>> &disjunctive_pre,
        std::vector<std::vector<bool>> &reached,
        const Landmark &landmark);

    virtual void generate_relaxed_landmarks(
        const std::shared_ptr<AbstractTask> &task,
        Exploration &exploration) override;
    void found_simple_lm_and_order(const FactPair &a, LandmarkNode &b,
                                   EdgeType t);
    void found_disj_lm_and_order(const TaskProxy &task_proxy,
                                 const std::set<FactPair> &a,
                                 LandmarkNode &b,
                                 EdgeType t);
    void approximate_lookahead_orders(const TaskProxy &task_proxy,
                                      const std::vector<std::vector<bool>> &reached,
                                      LandmarkNode *lmp);
    bool domain_connectivity(const State &initial_state,
                             const FactPair &landmark,
                             const std::unordered_set<int> &exclude);

    void build_disjunction_classes(const TaskProxy &task_proxy);

    void discard_disjunctive_landmarks();
public:
    LandmarkFactoryRpgSasp(bool disjunctive_landmarks, bool use_orders,
                           utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
