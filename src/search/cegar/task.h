#ifndef CEGAR_TASK_H
#define CEGAR_TASK_H

#include "utils.h"

#include "../global_operator.h"
#include "../global_state.h"

#include <set>
#include <string>
#include <vector>

namespace cegar_heuristic {
typedef std::unordered_set<Fact, hash_fact> FactSet;

class Task {
private:
    std::vector<int> initial_state_data;
    std::vector<Fact> goal;
    std::vector<int> variable_domain;
    std::vector<std::unordered_set<int> > unreachable_facts;
    std::vector<std::vector<std::string> > fact_names;
    std::vector<GlobalOperator> operators;
    std::vector<int> original_operator_numbers;
    std::vector<std::vector<int> > orig_index;
    std::vector<std::vector<int> > task_index;
    mutable AdditiveHeuristic *additive_heuristic;

    void move_fact(int var, int before, int after);
    void update_facts(int var, int num_values, const std::vector<int> &new_task_index);
    void find_and_apply_new_fact_ordering(int var, std::set<int> &unordered_values, int value_for_rest);
    void remove_unmarked_operators();

    void setup_hadd();

    void dump_name() const;
    void dump_facts() const;

public:
    Task(std::vector<int> domain, std::vector<std::vector<std::string> > names,
         std::vector<GlobalOperator> ops, std::vector<int> initial_state_data_,
         std::vector<Fact> goal_facts);

    const std::vector<Fact> &get_goal() const {return goal; }
    const std::vector<GlobalOperator> &get_operators() const {return operators; }

    void set_goal(const Fact &fact);
    void compute_possibly_before_facts(const Fact &last_fact, FactSet *reached);
    void save_unreachable_facts(const FactSet &reached_facts);
    // Only keep operators with all preconditions in reachable set of facts.
    void remove_inapplicable_operators(const FactSet reachable_facts);
    void keep_single_effect(const Fact &last_fact);
    void adapt_operator_costs(const std::vector<int> &remaining_costs);
    void adapt_remaining_costs(std::vector<int> &remaining_costs, const std::vector<int> &needed_costs) const;
    bool translate_state(const GlobalState &state, int *translated) const;

    void combine_facts(int var, std::unordered_set<int> &values);

    void install();
    void release_memory();

    static Task get_original_task();

    int get_hadd_value(int var, int value) const;
    int get_num_vars() const {return variable_domain.size(); }
    int get_num_values(int var) const {return variable_domain[var]; }
    const std::vector<Fact> &get_goals() const {return goal; }
    const std::vector<int> &get_initial_state_data() const {return initial_state_data; }
    const std::vector<int> &get_variable_domain() const {return variable_domain; }
    const std::vector<std::unordered_set<int> > &get_unreachable_facts() const {return unreachable_facts; }

    int get_projected_index(int var, int value) const {return task_index[var][value]; }

    double get_state_space_fraction(const Task &global_task) const;

    void dump() const;
};
}

#endif
