#ifndef ABSTRACT_TASK_H
#define ABSTRACT_TASK_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

class GlobalOperator;
class GlobalState;

namespace options {
class Options;
}

struct Fact {
    int var;
    int value;

    Fact(int var, int value)
      : var(var), value(value) {
    }

    bool operator<(const Fact &other) const {
        return get_pair() < other.get_pair();
    }

    std::pair<int, int> get_pair() const {
        return std::make_pair(var, value);
    }
};

class AbstractTask {
public:
    AbstractTask() = default;
    virtual ~AbstractTask() = default;
    virtual int get_num_variables() const = 0;
    virtual const std::string &get_variable_name(int var) const = 0;
    virtual int get_variable_domain_size(int var) const = 0;
    virtual const std::string &get_fact_name(int var, int value) const = 0;
    virtual bool are_facts_mutex(const Fact &fact1, const Fact &fact2) const = 0;

    virtual int get_operator_cost(int index, bool is_axiom) const = 0;
    virtual const std::string &get_operator_name(int index, bool is_axiom) const = 0;
    virtual int get_num_operators() const = 0;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const = 0;
    virtual Fact get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual Fact get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const = 0;
    virtual Fact get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual const GlobalOperator *get_global_operator(int index, bool is_axiom) const = 0;

    virtual int get_num_axioms() const = 0;

    virtual int get_num_goals() const = 0;
    virtual Fact get_goal_fact(int index) const = 0;

    virtual std::vector<int> get_initial_state_values() const = 0;
    virtual std::vector<int> get_state_values(const GlobalState &global_state) const = 0;
};

const std::shared_ptr<AbstractTask> get_task_from_options(
    const options::Options &opts);

#endif
