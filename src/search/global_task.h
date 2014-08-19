#ifndef GLOBAL_TASK_H
#define GLOBAL_TASK_H

#include "globals.h"
#include "operator.h"
#include "operator_cost.h"
#include "task.h"

#include <cassert>
#include <cstddef>


class GlobalTaskInterface : public TaskInterface {
public:
    GlobalTaskInterface();
    ~GlobalTaskInterface();

    int get_operator_cost(std::size_t index) const {
        assert(index < g_operators.size());
        return g_operators[index].get_cost();
    }
    int get_adjusted_operator_cost(std::size_t index, OperatorCost cost_type) const {
        assert(index < g_operators.size());
        return get_adjusted_action_cost(g_operators[index], cost_type);
    }
    std::size_t get_num_operators() const {return g_operators.size(); }
    std::size_t get_num_axioms() const {return g_axioms.size(); }
};

#endif
