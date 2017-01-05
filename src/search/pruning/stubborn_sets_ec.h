#ifndef PRUNING_STUBBORN_SETS_EC_H
#define PRUNING_STUBBORN_SETS_EC_H

#include "stubborn_sets.h"

namespace stubborn_sets_ec {
class StubbornSetsEC : public stubborn_sets::StubbornSets {
private:
    std::vector<std::vector<std::vector<bool>>> reachability_map;
    std::vector<std::vector<int>> op_preconditions_on_var;
    std::vector<bool> active_ops;
    std::vector<std::vector<ActionID>> conflicting_and_disabling;
    std::vector<std::vector<ActionID>> disabled;
    std::vector<bool> written_vars;
    std::vector<std::vector<bool>> nes_computed;

    bool is_applicable(ActionID op_id, const State &state) const;
    void get_disabled_vars(ActionID op1_id, ActionID op2_id,
                           std::vector<int> &disabled_vars) const;
    void build_reachability_map(const TaskProxy &task_proxy);
    void compute_operator_preconditions(const TaskProxy &task_proxy);
    void compute_conflicts_and_disabling();
    void add_conflicting_and_disabling(ActionID op_id, const State &state);
    void compute_active_operators(const State &state);
    void mark_as_stubborn_and_remember_written_vars(ActionID op_id, const State &state);
    void add_nes_for_fact(const FactPair &fact, const State &state);
    void apply_s5(ActionID op_id, const State &state);
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state, ActionID op_id) override;
public:
    virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
};
}
#endif
