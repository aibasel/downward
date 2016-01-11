#ifndef POR_STUBBORN_SETS_EC_H
#define POR_STUBBORN_SETS_EC_H

#include "stubborn_sets.h"

namespace stubborn_sets_ec {
using stubborn_sets::Fact;

struct StubbornDTG {
    struct Arc {
        Arc(int target_value, int operator_no)
            : target_value(target_value),
              operator_no(operator_no) {
        }
        int target_value;
        int operator_no;
    };

    struct Node {
        std::vector<Arc> outgoing;
        std::vector<Arc> incoming;
    };

    std::vector<Node> nodes;
    std::vector<bool> goal_values;

    void recurse_forwards(int value, std::vector<bool> &reachable) const;
public:
    void forward_reachability_analysis(int start_value,
                                       std::vector<bool> &reachable) const;
};

class StubbornSetsEC : public stubborn_sets::StubbornSets {
private:
    std::vector<StubbornDTG> dtgs;
    std::vector<std::vector<std::vector<bool>>> reachability_map;
    std::vector<std::vector<int>> operator_preconditions;
    std::vector<bool> active_ops;
    std::vector<std::vector<int>> conflicting_and_disabling;
    std::vector<std::vector<int>> disabled;
    std::vector<bool> written_vars;
    std::vector<std::vector<bool>> nes_computed;

    void get_disabled_vars(int op1_no, int op2_no, std::vector<int> &disabled_vars);
    void build_dtgs();
    void build_reachability_map();
    void compute_operator_preconditions();
    void compute_conflicts_and_disabling();
    void compute_disabled_by_o();
    void add_conflicting_and_disabling(int op_no, const GlobalState &state);
    void recurse_forwards(int var, int start_value, int current_value, std::vector<bool> &reachable);
    void compute_active_operators(const GlobalState &state);
    void mark_as_stubborn(int op_no, const GlobalState &state);
    void add_nes_for_fact(Fact fact, const GlobalState &state);
    void apply_s5(const GlobalOperator &op, const GlobalState &state);
protected:
    virtual void initialize();
    virtual void compute_stubborn_set(const GlobalState &state, std::vector<const GlobalOperator *> &ops);
public:
    StubbornSetsEC();
    ~StubbornSetsEC();
};
}
#endif
