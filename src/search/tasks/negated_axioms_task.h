#ifndef TASKS_NEGATED_AXIOMS_TASK_H
#define TASKS_NEGATED_AXIOMS_TASK_H

#include "delegating_task.h"

#include <set>

namespace plugins {
class Options;
}

/*
  This task transformation adds explicit axioms for how the default value
  of derived variables can be achieved. In general this is done as follows:
  Given derived variable v with n axioms v <- c_1, ..., v <- c_n, add axioms
  that together represent ¬v <- ¬c_1 ^ ... ^ ¬c_n.

  Notes:
   - THE TRANSFORMATION CAN BE SLOW! The rule ¬v <- ¬c_1 ^ ... ^ ¬c_n must
   be split up into axioms whose conditions are simple conjunctions. Since
   all c_i are also simple conjunctions, this amounts to converting a CNF
   to a DNF.
   - To address the potential exponential blowup, we provide an option
   to instead add trivial default values axioms, i.e. axioms that assign
   the default value and have an empty body. This guarantees short runtime
   but loses information.
   - The transformation is not exact. For derived variables v that have cyclic
   dependencies, the general approach is incorrect. We instead trivially
   overapproximate such cases with an axiom with an empty body.
   - The search ignores axioms that set the derived variable to their default
   value. The task transformation is thus only meant for heuristics that need
   to know how to achieve the default value.
 */

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
        std::set<FactPair> &hitting_set,
        std::unordered_set<int> &hitting_set_vars,
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
