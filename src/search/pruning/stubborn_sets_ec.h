#ifndef PRUNING_STUBBORN_SETS_EC_H
#define PRUNING_STUBBORN_SETS_EC_H

#include "stubborn_sets.h"

namespace stubborn_sets_ec {
class StubbornSetsEC : public stubborn_sets::StubbornSets {
private:
    std::vector<std::vector<std::vector<bool>>> reachability_map;
    std::vector<std::vector<int>> op_preconditions_on_var;
    std::vector<bool> active_ops;
    std::vector<std::vector<int>> conflicting_and_disabling;
    std::vector<std::vector<int>> disabled;
    std::vector<bool> written_vars;
    std::vector<std::vector<bool>> nes_computed;

    void get_disabled_vars(OperatorProxy op1, OperatorProxy op2,
                           std::vector<int> &disabled_vars);
    void build_reachability_map();
    void compute_operator_preconditions();
    void compute_conflicts_and_disabling();
    void add_conflicting_and_disabling(OperatorProxy op, const State &state);
    void compute_active_operators(const State &state);
    void mark_as_stubborn_and_remember_written_vars(int op_no, const State &state);
    void add_nes_for_fact(Fact fact, const State &state);
    void apply_s5(OperatorProxy op, const State &state);
protected:
    virtual void initialize_stubborn_set(const State &state) override;
    virtual void handle_stubborn_operator(const State &state,
                                          OperatorProxy op) override;
public:
    StubbornSetsEC();
    virtual ~StubbornSetsEC() = default;
};
}
#endif
