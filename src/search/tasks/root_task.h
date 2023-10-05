#ifndef TASKS_ROOT_TASK_H
#define TASKS_ROOT_TASK_H

#include "../abstract_task.h"

#include <set>

namespace tasks {
extern std::shared_ptr<AbstractTask> g_root_task;
extern void read_root_task(std::istream &in);


struct ExplicitVariable {
    int domain_size;
    std::string name;
    std::vector<std::string> fact_names;
    int axiom_layer;
    int axiom_default_value;

    explicit ExplicitVariable(std::istream &in);
};


struct ExplicitEffect {
    FactPair fact;
    std::vector<FactPair> conditions;

    ExplicitEffect(int var, int value, std::vector<FactPair> &&conditions);
};


struct ExplicitOperator {
    std::vector<FactPair> preconditions;
    std::vector<ExplicitEffect> effects;
    int cost;
    std::string name;
    bool is_an_axiom;

    void read_pre_post(std::istream &in);
    ExplicitOperator(std::istream &in, bool is_an_axiom, bool use_metric);
};



class RootTask : public AbstractTask {
    std::vector<ExplicitVariable> variables;
    // TODO: think about using hash sets here.
    std::vector<std::vector<std::set<FactPair>>> mutexes;
    std::vector<ExplicitOperator> operators;
    std::vector<ExplicitOperator> axioms;
    std::vector<int> initial_state_values;
    std::vector<FactPair> goals;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const;

public:
    explicit RootTask(std::istream &in);

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
    virtual int convert_operator_index(
            int index, const AbstractTask *ancestor_task) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual void convert_ancestor_state_values(
            std::vector<int> &values,
            const AbstractTask *ancestor_task) const override;
};


class TaskIndependentRootTask : public TaskIndependentAbstractTask {
public:
    explicit TaskIndependentRootTask();

    virtual std::shared_ptr<AbstractTask> create_task_specific_AbstractTask(
            const std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<RootTask> create_task_specific_RootTask(const std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<RootTask> create_task_specific_RootTask(const std::shared_ptr<AbstractTask> &task, std::unique_ptr<ComponentMap> &component_map, int depth = -1);
};

}
#endif
