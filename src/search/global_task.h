#ifndef GLOBAL_TASK_H
#define GLOBAL_TASK_H

#include "globals.h"
#include "operator.h"
#include "operator_cost.h"
#include "task.h"

#include <cassert>
#include <cstddef>
#include <utility>


class GlobalTaskInterface : public TaskInterface {
public:
    GlobalTaskInterface();
    ~GlobalTaskInterface();

    std::size_t get_num_variables() const {return g_variable_domain.size(); }
    std::size_t get_variable_domain_size(std::size_t id) const {return g_variable_domain[id]; }
    int get_operator_cost(std::size_t index) const {
        assert(index < g_operators.size());
        return g_operators[index].get_cost();
    }
    int get_adjusted_operator_cost(std::size_t index, OperatorCost cost_type) const {
        assert(index < g_operators.size());
        return get_adjusted_action_cost(g_operators[index], cost_type);
    }
    std::size_t get_num_operators() const {return g_operators.size(); }
    std::size_t get_operator_precondition_size(std::size_t index) const;
    std::pair<std::size_t, std::size_t> get_operator_precondition_fact(std::size_t op_index, std::size_t fact_index) const;
    std::size_t get_num_axioms() const {return g_axioms.size(); }
};

#endif
