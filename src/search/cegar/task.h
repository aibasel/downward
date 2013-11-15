#ifndef CEGAR_TASK_H
#define CEGAR_TASK_H

#include "utils.h"
#include "../operator.h"
#include "../state.h"

#include <set>
#include <vector>

namespace cegar_heuristic {
typedef std::unordered_set<Fact, hash_fact> FactSet;

class Task {
private:
    State initial_state;
    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<vector<string> > fact_names;
    std::vector<Operator> operators;
    std::vector<int> original_operator_numbers;
    std::vector<std::vector<int> > orig_index;
    std::vector<std::vector<int> > task_index;
    mutable AdditiveHeuristic *additive_heuristic;
    bool is_original_task;

    void move_fact(int var, int before, int after);
    void update_facts(int var, int num_values, const std::vector<int> &new_task_index);
    void compute_possibly_before_facts(const Fact &last_fact, FactSet *reached);
    void compute_facts_and_operators();
    void find_and_apply_new_fact_ordering(int var, std::set<int> &unordered_values, int value_for_rest);
    void remove_unreachable_facts(const FactSet &reached_facts);
    void remove_unmarked_operators();
    void remove_inapplicable_operators(const FactSet reachable_facts);
    void mark_relevant_operators(const Fact &fact);

    void setup_hadd() const;

    void dump_name() const;
    void dump_facts() const;

public:
    explicit Task(std::vector<int> domain, std::vector<std::vector<std::string> > names,
                  std::vector<Operator> ops, State init, std::vector<Fact> goal_facts);

    const std::vector<Fact> &get_goal() const {return goal; }
    const std::vector<Operator> &get_operators() const {return operators; }

    void remove_irrelevant_operators();
    void set_goal(const Fact &fact, bool adapt);
    void adapt_operator_costs(const vector<int> &remaining_costs);
    void adapt_remaining_costs(vector<int> &remaining_costs, const vector<int> &needed_costs) const;
    bool translate_state(State &state) const;

    void combine_facts(int var, unordered_set<int> &values);

    void install();
    void release_memory();

    static Task get_original_task();

    void remove_hadd() const {additive_heuristic = 0; }
    int get_hadd_estimate_for_initial_state() const;
    int get_hadd_value(int var, int value) const;

    void dump() const;
};
}

#endif
