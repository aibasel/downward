#ifndef CEGAR_TASK_H
#define CEGAR_TASK_H

#include "utils.h"
#include "../operator.h"
#include "../state.h"

#include <unordered_set>
#include <vector>

namespace cegar_heuristic {
class Task {
private:

public:
    Task();

    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<Operator> operators;
    std::vector<int> original_operator_numbers;
    std::unordered_set<int> fact_numbers;

    void install();
    void release_memory();

    static Task get_original_task();

    void dump_facts() const;
};
}

#endif
