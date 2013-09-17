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
    std::vector<std::vector<int> > fact_mapping;

    void rename_fact(int var, int before, int after);
    void remove_fact(const Fact &fact);

public:
    Task();

    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<Operator> operators;
    std::vector<int> original_operator_numbers;
    std::unordered_set<int> fact_numbers;

    void remove_unreachable_facts();
    void combine_facts(int var, const vector<int> &values);

    void install();
    void release_memory();

    static Task get_original_task();

    void dump_facts() const;
};
}

#endif
