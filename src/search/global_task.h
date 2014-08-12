#ifndef GLOBAL_TASK_H
#define GLOBAL_TASK_H

#include "globals.h"
#include "operator.h"
#include "task.h"

#include <cassert>
#include <cstddef>


class GlobalTaskImpl : public TaskImpl {
public:
    int get_operator_cost(std::size_t index) const {
        assert(index < g_operators.size());
        return g_operators[index].get_cost();
    }
    std::size_t get_num_operators() const {return g_operators.size(); }
};

#endif
