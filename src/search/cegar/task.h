#ifndef CEGAR_TASK_H
#define CEGAR_TASK_H

#include "utils.h"
#include "../operator.h"
#include "../state.h"

#include <unordered_set>
#include <vector>

namespace cegar_heuristic {

typedef std::unordered_set<Fact, hash_fact> FactSet;

class Task {
private:
    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<vector<string> > fact_names;
    std::vector<Operator> operators;
    std::vector<int> original_operator_numbers;
    std::vector<std::vector<int> > fact_mapping;

    void move_fact(int var, int before, int after);
    void remove_fact(int var, int value);
    void remove_facts(int var, vector<int> &values);
    void compute_possibly_before_facts(const Fact &last_fact, FactSet *reached);
    void compute_facts_and_operators();
    void remove_unreachable_facts(const FactSet &reached_facts);
    void mark_relevant_operators(const Fact &fact);

    void dump_facts() const;

public:
    explicit Task(std::vector<Fact> goal_facts);

    const std::vector<Fact> &get_goal() const {return goal; }
    std::vector<Operator> &get_operators() {return operators; }

    void remove_irrelevant_operators();
    void adapt_operator_costs(const vector<int> &remaining_costs);
    void adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) const;
    void translate_state(State &state, bool &reachable) const;

    void combine_facts(int var, vector<int> &values);

    void install();
    void release_memory();

    static Task get_original_task();

    void dump() const;
};
}

#endif
