#ifndef TASKS_NEGATED_AXIOMS_TASK_H
#define TASKS_NEGATED_AXIOMS_TASK_H

#include "delegating_task.h"

#include <set>

namespace plugins {
class Options;
}

namespace tasks {
struct NegatedAxiom {
    FactPair head;
    std::vector<FactPair> condition;

    NegatedAxiom(FactPair head, std::vector<FactPair> &&condition)
        : head(head), condition(condition) {}
};

class NegatedAxiomsTask : public DelegatingTask {
    bool simple_default_axioms;
    std::vector<NegatedAxiom> negated_axioms;
    int negated_axioms_start_index;

    std::unordered_set<int> collect_needed_negatively(
        const std::vector<std::vector<int>> &positive_dependencies,
        const std::vector<std::vector<int>> &negative_dependencies,
        const std::vector<std::vector<int> *> &var_to_scc);
    void add_negated_axioms_for_var(
        FactPair head, std::vector<int> &axiom_ids);
    void collect_non_dominated_hitting_sets_recursively(
        const std::vector<std::set<FactPair>> &conditions_as_cnf, size_t index,
        std::set<FactPair> &hitting_set, std::set<int> &hitting_set_vars,
        std::set<std::set<FactPair>> &results);
public:
    explicit NegatedAxiomsTask(
        const std::shared_ptr<AbstractTask> &parent,
        bool simple_default_axioms);
    virtual ~NegatedAxiomsTask() override = default;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual std::string get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual int get_num_axioms() const override;
};
}

#endif
