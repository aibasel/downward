#ifndef CEGAR_LANDMARK_TASK_H
#define CEGAR_LANDMARK_TASK_H

#include "utils.h"

#include "../delegating_task.h"
#include "../global_operator.h"
#include "../global_state.h"

#include <set>
#include <string>
#include <vector>

namespace cegar {
typedef std::unordered_set<Fact, hash_fact> FactSet;

class LandmarkTask : DelegatingTask {
private:
    std::vector<int> initial_state_data;
    std::vector<Fact> goals;
    std::vector<int> variable_domain;
    std::vector<std::unordered_set<int> > unreachable_facts;
    std::vector<std::vector<std::string> > fact_names;
    std::vector<GlobalOperator> operators;
    std::vector<int> op_costs;
    std::vector<std::vector<int> > orig_index;
    std::vector<std::vector<int> > task_index;
    mutable AdditiveHeuristic *additive_heuristic;

    void move_fact(int var, int before, int after);
    void update_facts(int var, int num_values, const std::vector<int> &new_task_index);
    void find_and_apply_new_fact_ordering(int var, std::set<int> &unordered_values, int value_for_rest);

    void setup_hadd();

    void dump_name() const;
    void dump_facts() const;

    int translate_fact(int var, int value);
    int get_orig_op_index(int index) const;

public:
    LandmarkTask(std::vector<int> domain, std::vector<std::vector<std::string> > names,
         std::vector<GlobalOperator> ops, std::vector<int> initial_state_data_,
         std::vector<Fact> goal_facts);
    LandmarkTask(std::shared_ptr<AbstractTask> parent,
                 FactProxy landmark,
                 const VariableToValues &fact_groups);

    const std::vector<Fact> &get_goal() const {return goals; }
    const std::vector<GlobalOperator> &get_operators() const {return operators; }

    void set_goal(const Fact &fact);
    void save_unreachable_facts(VariablesProxy variables, const std::unordered_set<FactProxy> &reachable_facts);
    void adapt_operator_costs(const std::vector<int> &remaining_costs);
    void adapt_remaining_costs(std::vector<int> &remaining_costs, const std::vector<int> &needed_costs) const;
    bool translate_state(const GlobalState &state, int *translated) const;

    void combine_facts(int var, const std::unordered_set<int> &values);

    void install();
    void release_memory();

    static LandmarkTask get_original_task();

    int get_hadd_value(int var, int value) const;
    int get_num_vars() const {return variable_domain.size(); }
    int get_num_values(int var) const {return variable_domain[var]; }
    const std::vector<Fact> &get_goals() const {return goals; }
    const std::vector<int> &get_initial_state_data() const {return initial_state_data; }
    const std::vector<int> &get_variable_domain() const {return variable_domain; }
    const std::vector<std::unordered_set<int> > &get_unreachable_facts() const {return unreachable_facts; }

    int get_projected_index(int var, int value) const {return task_index[var][value]; }

    double get_state_space_fraction(const LandmarkTask &global_task) const;

    void dump() const;


    virtual int get_variable_domain_size(int var) const override;
    virtual const std::string &get_fact_name(int var, int value) const override;

    virtual const std::string &get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
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

    virtual int get_num_goals() const override;
    virtual std::pair<int, int> get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
    virtual std::vector<int> get_state_values(const GlobalState &global_state) const override;
};
}

#endif
