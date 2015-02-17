#ifndef GLOBAL_TASK_INTERFACE_H
#define GLOBAL_TASK_INTERFACE_H

#include "global_operator.h"
#include "globals.h"
#include "task_interface.h"
#include "utilities.h"

#include <cassert>
#include <utility>


class GlobalTaskInterface : public TaskInterface {
private:
    const GlobalOperator &get_operator_or_axiom(int index, bool is_axiom) const {
        if (is_axiom) {
            assert(in_bounds(index, g_axioms));
            return g_axioms[index];
        } else {
            assert(in_bounds(index, g_operators));
            return g_operators[index];
        }
    }

public:
    virtual int get_num_variables() const override {
        return g_variable_domain.size();
    }
    virtual int get_variable_domain_size(int var) const override {
        return g_variable_domain[var];
    }

    virtual int get_operator_cost(int index, bool is_axiom) const override {
        return get_operator_or_axiom(index, is_axiom).get_cost();
    }
    virtual const std::string &get_operator_name(int index, bool is_axiom) const override {
        return get_operator_or_axiom(index, is_axiom).get_name();
    }
    virtual int get_num_operators() const override {
        return g_operators.size();
    }
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual std::pair<int, int> get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual const GlobalOperator *get_global_operator(int index, bool is_axiom) const override;

    virtual int get_num_axioms() const override {
        return g_axioms.size();
    }

    virtual int get_num_goals() const override {
        return g_goal.size();
    }
    virtual std::pair<int, int> get_goal_fact(int index) const override {
        return g_goal[index];
    }
};

#endif
