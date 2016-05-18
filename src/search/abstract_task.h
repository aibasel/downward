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
private:
    std::pair<int, int> get_pair() const {
        return std::make_pair(var, value);
    }

public:
    int var;
    int value;

    Fact(int var, int value)
        : var(var), value(value) {
    }

    bool operator<(const Fact &other) const {
        return get_pair() < other.get_pair();
    }

    bool operator==(const Fact &other) const {
        return get_pair() == other.get_pair();
    }

    bool operator!=(const Fact &other) const {
        return !(*this == other);
    }
};

class AbstractTask {
public:
    AbstractTask() = default;
    virtual ~AbstractTask() = default;
    virtual int get_num_variables() const = 0;
    virtual const std::string &get_variable_name(int var) const = 0;
    virtual int get_variable_domain_size(int var) const = 0;
    virtual const std::string &get_fact_name(const Fact &fact) const = 0;
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
    /*
      Convert state values from an ancestor task A (ancestor) into
      state values from this task, C (child). Task A has to be an
      ancestor of C in the sense that C is the result of a sequence of
      task transformations on A.
      The values are converted in-place to avoid unnecessary copies. If a
      subclass needs to create a new vector, e.g., because the size changes,
      it should create the new vector in a local variable and then swap it with
      the parameter.
    */
    virtual void convert_state_values(
        std::vector<int> &values,
        const AbstractTask *ancestor_task) const = 0;
};

const std::shared_ptr<AbstractTask> get_task_from_options(
    const options::Options &opts);

#endif
