#ifndef TASKS_ROOT_TASK_H
#define TASKS_ROOT_TASK_H

#include "../abstract_task.h"
#include "../global_operator.h"

#include "../utils/collections.h"

#include <set>
#include <vector>

namespace tasks {
struct ExplicitEffect;
struct ExplicitOperator;
struct ExplicitVariable;

class RootTask : public AbstractTask {
    const std::vector<ExplicitVariable> variables;
    const std::vector<std::vector<std::set<FactPair>>> mutexes;
    const std::vector<ExplicitOperator> operators;
    const std::vector<ExplicitOperator> axioms;
    mutable std::vector<int> initial_state_values;
    const std::vector<FactPair> goals;
    mutable bool evaluated_axioms_on_initial_state;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const;

    bool run_sanity_check() const;
    void evaluate_axioms_on_initial_state() const;

public:
    RootTask(
        std::vector<ExplicitVariable> &&variables,
        std::vector<std::vector<std::set<FactPair>>> &&mutexes,
        std::vector<ExplicitOperator> &&operators,
        std::vector<ExplicitOperator> &&axioms,
        std::vector<int> &&initial_state_values,
        std::vector<FactPair> &&goals);

    virtual int get_num_variables() const override;
    virtual std::string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual std::string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual std::string get_operator_name(
        int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(
        int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(
        int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual OperatorID get_global_operator_id(OperatorID id) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual void convert_state_values(
        std::vector<int> &values,
        const AbstractTask *ancestor_task) const override;
};


/*
  Eventually, this should parse the task from a stream.
  this is currently done by the methods read_* in globals.
  We have to keep them as long as there is still code that
  accesses the global variables that store the task. For
  now, we let those methods do the parsing and access the
  parsed task here.
*/
std::shared_ptr<RootTask> create_root_task(
    const std::vector<std::string> &variable_name,
    const std::vector<int> &variable_domain,
    const std::vector<std::vector<std::string>> &fact_names,
    const std::vector<int> &axiom_layers,
    const std::vector<int> &default_axiom_values,
    const std::vector<std::vector<std::set<FactPair>>> &inconsistent_facts,
    const std::vector<int> &initial_state_data,
    const std::vector<std::pair<int, int>> &goal,
    const std::vector<GlobalOperator> &operators,
    const std::vector<GlobalOperator> &axioms);


/*
  The following classes are meant for internal use only. We currently cannot
  move them into the cc file because we have vectors of them in RootTask.
*/

struct ExplicitVariable {
    const int domain_size;
    const std::string name;
    const std::vector<std::string> fact_names;
    const int axiom_layer;
    const int axiom_default_value;

    ExplicitVariable(
        int domain_size,
        std::string &&name,
        std::vector<std::string> &&fact_names,
        int axiom_layer,
        int axiom_default_value);
};


struct ExplicitEffect {
    const FactPair fact;
    const std::vector<FactPair> conditions;

    ExplicitEffect(int var, int value, std::vector<FactPair> &&conditions);
};


struct ExplicitOperator {
    const std::vector<FactPair> preconditions;
    const std::vector<ExplicitEffect> effects;
    const int cost;
    const std::string name;
    const bool is_an_axiom;

    ExplicitOperator(
        std::vector<FactPair> &&preconditions,
        std::vector<ExplicitEffect> &&effects,
        int cost,
        const std::string &name,
        bool is_an_axiom);
};
}
#endif
