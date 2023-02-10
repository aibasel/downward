#ifndef PRUNING_STUBBORN_SETS_EC_H
#define PRUNING_STUBBORN_SETS_EC_H

#include "stubborn_sets_action_centric.h"

namespace stubborn_sets_ec {
class StubbornSetsEC : public stubborn_sets::StubbornSetsActionCentric {
private:
    std::vector<std::vector<std::vector<bool>>> reachability_map;
    std::vector<std::vector<int>> op_preconditions_on_var;
    std::vector<bool> active_ops;
    std::vector<std::vector<int>> conflicting_and_disabling;
    std::vector<bool> conflicting_and_disabling_computed;
    std::vector<std::vector<int>> disabled;
    std::vector<bool> disabled_computed;
    std::vector<bool> written_vars;
    std::vector<std::vector<bool>> nes_computed;

    bool is_applicable(int op_no, const State &state) const;
    void get_disabled_vars(int op1_no, int op2_no,
                           std::vector<int> &disabled_vars) const;
    void build_reachability_map(const TaskProxy &task_proxy);
    void compute_operator_preconditions(const TaskProxy &task_proxy);
    const std::vector<int> &get_conflicting_and_disabling(int op1_no);
    const std::vector<int> &get_disabled(int op1_no);
    void add_conflicting_and_disabling(int op_no, const State &state);
    void compute_active_operators(const State &state);
    void enqueue_stubborn_operator_and_remember_written_vars(int op_no, const State &state);
    void add_nes_for_fact(const FactPair &fact, const State &state);
    void apply_s5(int op_no, const State &state);
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state, int op_no) override;
public:
    explicit StubbornSetsEC(const plugins::Options &opts);
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};
}
#endif
