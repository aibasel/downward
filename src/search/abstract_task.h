#ifndef ABSTRACT_TASK_H
#define ABSTRACT_TASK_H

#include "operator_id.h"

#include "algorithms/subscriber.h"
#include "utils/hash.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace options {
class Options;
}

struct FactPair {
    int var;
    int value;

    FactPair(int var, int value)
        : var(var), value(value) {
    }

    bool operator<(const FactPair &other) const {
        return var < other.var || (var == other.var && value < other.value);
    }

    bool operator==(const FactPair &other) const {
        return var == other.var && value == other.value;
    }

    bool operator!=(const FactPair &other) const {
        return var != other.var || value != other.value;
    }

    /*
      This special object represents "no such fact". E.g., functions
      that search a fact can return "no_fact" when no matching fact is
      found.
    */
    static const FactPair no_fact;
};

std::ostream &operator<<(std::ostream &os, const FactPair &fact_pair);

namespace utils {
inline void feed(HashState &hash_state, const FactPair &fact) {
    feed(hash_state, fact.var);
    feed(hash_state, fact.value);
}
}

class AbstractTask : public subscriber::SubscriberService<AbstractTask> {
public:
    AbstractTask() = default;
    virtual ~AbstractTask() override = default;
    virtual int get_num_variables() const = 0;
    virtual std::string get_variable_name(int var) const = 0;
    virtual int get_variable_domain_size(int var) const = 0;
    virtual int get_variable_axiom_layer(int var) const = 0;
    virtual int get_variable_default_axiom_value(int var) const = 0;
    virtual std::string get_fact_name(const FactPair &fact) const = 0;
    virtual bool are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const = 0;

    virtual int get_operator_cost(int index, bool is_axiom) const = 0;
    virtual std::string get_operator_name(int index, bool is_axiom) const = 0;
    virtual int get_num_operators() const = 0;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const = 0;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const = 0;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const = 0;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const = 0;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const = 0;

    /*
      Convert an operator index from this task, C (child), into an operator index
      from an ancestor task A (ancestor). Task A has to be an ancestor of C in
      the sense that C is the result of a sequence of task transformations on A.
    */
    virtual int convert_operator_index(
        int index, const AbstractTask *ancestor_task) const = 0;

    virtual int get_num_axioms() const = 0;

    virtual int get_num_goals() const = 0;
    virtual FactPair get_goal_fact(int index) const = 0;

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
    virtual void convert_ancestor_state_values(
        std::vector<int> &values,
        const AbstractTask *ancestor_task) const = 0;
};

#endif
