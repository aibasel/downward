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
    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<vector<string> > fact_names;
    std::vector<Operator> operators;
    std::vector<int> original_operator_numbers;
    std::unordered_set<int> fact_numbers;
    std::vector<std::vector<int> > fact_mapping;

    void rename_fact(int var, int before, int after);
    void remove_fact(const Fact &fact);
    void shrink_domain(int var, int shrink_by);
    void compute_possibly_before_facts(const Fact &last_fact);
    void compute_facts_and_operators();
    void remove_unreachable_facts();
    void mark_relevant_operators(const Fact &fact);

    void dump_facts() const;

public:
    Task();

    const std::vector<Fact> &get_goal() const {return goal; }
    void set_goal(std::vector<Fact> facts);

    std::vector<Operator> &get_operators() {return operators; }

    void remove_irrelevant_operators();
    void adapt_operator_costs(const vector<int> &remaining_costs);
    void adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) const;
    bool state_is_reachable(const State &state) const;
    void project_state(State &state) const;

    void combine_facts(int var, const vector<int> &values);

    void install();
    void release_memory();

    static Task get_original_task();

    void dump() const;
};
}

#endif
