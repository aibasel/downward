#ifndef TASKS_DEFAULT_VALUE_AXIOMS_TASK_H
#define TASKS_DEFAULT_VALUE_AXIOMS_TASK_H

#include "delegating_task.h"

#include "../plugins/plugin.h"

#include <set>

/*
  This task transformation adds explicit axioms for how the default value
  of derived variables can be achieved. In general this is done as follows:
  Given derived variable v with n axioms v <- c_1, ..., v <- c_n, add axioms
  that together represent ¬v <- ¬c_1 ^ ... ^ ¬c_n.

  Notes:
   - Technically, this transformation is illegal since it adds axioms which set
   derived variables to their default value. Do not use it for search; the
   axiom evaluator expects that all axioms set variables to their non-default
   values (checked with an assertion).
   - We assume that derived variables are binary.
   - THE TRANSFORMATION CAN BE SLOW! The rule ¬v <- ¬c_1 ^ ... ^ ¬c_n must
   be split up into axioms whose conditions are simple conjunctions. Since
   all c_i are also simple conjunctions, this amounts to converting a CNF
   to a DNF.
   - The transformation is not exact. For derived variables v that have cyclic
   dependencies, the general approach is incorrect. We instead trivially
   overapproximate such cases with "¬v <- T" (see issue453).
   - If you wish to avoid the combinatorial explosion from converting a
   CNF to a DNF, you can set the option "axioms" to "approximate_negative";
   this will use the trivial overapproximation "¬v <- T" for *all* derived
   variables. Note that this can negatively impact the heuristic values.
 */

namespace tasks {
enum class AxiomHandlingType {
    APPROXIMATE_NEGATIVE, APPROXIMATE_NEGATIVE_CYCLES
};

struct DefaultValueAxiom {
    FactPair head;
    std::vector<FactPair> condition;

    DefaultValueAxiom(FactPair head, std::vector<FactPair> &&condition)
        : head(head), condition(condition) {}
};

class DefaultValueAxiomsTask : public DelegatingTask {
    AxiomHandlingType axioms;
    std::vector<DefaultValueAxiom> default_value_axioms;
    int default_value_axioms_start_index;

    std::unordered_set<int> get_vars_with_relevant_default_value(
        const std::vector<std::vector<int>> &nondefault_dependencies,
        const std::vector<std::vector<int>> &default_dependencies,
        const std::vector<std::vector<int> *> &var_to_scc);
    void add_default_value_axioms_for_var(
        FactPair head, std::vector<int> &axiom_ids);
    void collect_non_dominated_hitting_sets_recursively(
        const std::vector<std::set<FactPair>> &set_of_sets, size_t index,
        std::set<FactPair> &hitting_set,
        std::unordered_set<int> &hitting_set_vars,
        std::set<std::set<FactPair>> &results);
public:
    explicit DefaultValueAxiomsTask(
        const std::shared_ptr<AbstractTask> &parent,
        AxiomHandlingType axioms);
    virtual ~DefaultValueAxiomsTask() override = default;

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

extern std::shared_ptr<AbstractTask> get_default_value_axioms_task_if_needed(
    const std::shared_ptr<AbstractTask> &task,
    AxiomHandlingType axioms);
extern void add_axioms_option_to_feature(plugins::Feature &feature);
extern std::tuple<AxiomHandlingType> get_axioms_arguments_from_options(
    const plugins::Options &opts);
}

#endif
