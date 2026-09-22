#ifndef LANDMARKS_LANDMARK_FACTORY_HM_H
#define LANDMARKS_LANDMARK_FACTORY_HM_H

#include "landmark_factory.h"

#include <map>
#include <set>
#include <vector>

namespace landmarks {
using Propositions = std::vector<FactPair>;

std::ostream &operator<<(std::ostream &os, const Propositions &fs);

struct PropositionSetComparer {
    bool operator()(const Propositions &fs1, const Propositions &fs2) const {
        if (fs1.size() != fs2.size()) {
            return fs1.size() < fs2.size();
        }
        for (size_t i = 0; i < fs1.size(); ++i) {
            if (fs1[i] != fs2[i]) {
                return fs1[i] < fs2[i];
            }
        }
        return false;
    }
};

struct ConditionalNoop {
    std::vector<int> effect_condition;
    std::vector<int> effect;

    ConditionalNoop(
        std::vector<int> &&effect_condition, std::vector<int> &&effect)
        : effect_condition(move(effect_condition)), effect(move(effect)) {
    }
};

/* Corresponds to an operator from the original problem, as well as a
   set of conditional effects that correspond to noops. */
struct PiMOperator {
    // Preconditions and effects reference the proposition IDs.
    std::vector<int> precondition;
    std::vector<int> effect;
    std::vector<ConditionalNoop> conditional_noops;
    int id;
};

// Represents a proposition in the P^m problem.
struct HMEntry {
    // Propositions that belong to this set.
    const Propositions propositions;
    bool reached;

    std::vector<int> landmarks;
    /*
      Landmarks that are "preconditions" to achieve this `HMEntry`. This
      set is disjoint from `landmarks` above and used to derive
      greedy-necessary orderings.
    */
    std::vector<int> precondition_landmarks;

    std::unordered_set<int> first_achievers;

    /*
      The first int represents an operator ID. If the second int is -1 it means
      the `propositions` are a precondition of the corresponding operator. If
      the second int is >= 0 it points to the respective conditional noop for
      which `propositions` occur in the effect condition.
    */
    std::vector<std::pair<int, int>> triggered_operators;

    explicit HMEntry(Propositions &&propositions)
        : propositions(move(propositions)), reached(false) {
    }
};

class LandmarkFactoryHM : public LandmarkFactory {
    using TriggerSet = std::unordered_map<int, std::unordered_set<int>>;

    const int m;
    const bool conjunctive_landmarks;
    const bool use_orders;

    std::unordered_map<int, LandmarkNode *> landmark_nodes;

    std::vector<HMEntry> hm_table;
    std::vector<PiMOperator> pm_operators;
    // Maps each set of < m propositions to an int representing its ID.
    std::map<Propositions, int, PropositionSetComparer> set_indices;
    /*
      The number in the first position represents the amount of unsatisfied
      preconditions of the operator. The vector of numbers in the second
      position represents the amount of unsatisfied preconditions for each
      conditional noop operator.
    */
    std::vector<std::pair<int, std::vector<int>>> num_unsatisfied_preconditions;

    std::unordered_set<int> collect_and_add_landmarks_to_landmark_graph(
        const VariablesProxy &variables, const Propositions &propositions);
    void reduce_landmarks(const std::unordered_set<int> &landmarks);
    void add_landmark_orderings(const std::unordered_set<int> &landmarks);
    void construct_landmark_graph(const TaskProxy &task_proxy);
    virtual void generate_landmarks(
        const std::shared_ptr<AbstractTask> &task) override;

    TriggerSet mark_state_propositions_reached(
        const State &state, const VariablesProxy &variables);
    void collect_condition_landmarks(
        const std::vector<int> &condition, std::vector<int> &landmarks) const;
    void initialize_proposition_landmark(
        int op_id, HMEntry &hm_entry, const std::vector<int> &landmarks,
        const std::vector<int> &precondition_landmarks, TriggerSet &triggers);
    void update_proposition_landmark(
        int op_id, int proposition, const std::vector<int> &landmarks,
        const std::vector<int> &precondition_landmarks, TriggerSet &triggers);
    void update_effect_landmarks(
        int op_id, const std::vector<int> &effect,
        const std::vector<int> &landmarks,
        const std::vector<int> &precondition_landmarks, TriggerSet &triggers);
    void update_noop_landmarks(
        const std::unordered_set<int> &current_triggers, const PiMOperator &op,
        const std::vector<int> &landmarks,
        const std::vector<int> &prerequisites, TriggerSet &next_triggers);
    void compute_noop_landmarks(
        int op_id, int noop_index, const std::vector<int> &local_landmarks,
        const std::vector<int> &local_necessary, TriggerSet &next_trigger);
    void compute_hm_landmarks(const TaskProxy &task_proxy);

    void trigger_operator(
        int op_id, bool newly_discovered, TriggerSet &trigger);
    void trigger_conditional_noop(
        int op_id, int noop_id, bool newly_discovered, TriggerSet &trigger);
    void propagate_pm_propositions(
        HMEntry &hm_entry, bool newly_discovered, TriggerSet &trigger);

    Propositions initialize_preconditions(
        const VariablesProxy &variables, const OperatorProxy &op,
        PiMOperator &pm_op);
    Propositions initialize_postconditions(
        const VariablesProxy &variables, const OperatorProxy &op,
        PiMOperator &pm_op);
    std::vector<int> compute_noop_precondition(
        const std::vector<Propositions> &preconditions, int op_id,
        int noop_index);
    std::vector<int> compute_noop_effect(
        const std::vector<Propositions> &postconditions);
    void add_conditional_noop(
        PiMOperator &pm_op, int op_id, const VariablesProxy &variables,
        const Propositions &propositions, const Propositions &preconditions,
        const Propositions &postconditions);
    void initialize_noops(
        const VariablesProxy &variables, PiMOperator &pm_op, int op_id,
        const Propositions &preconditions, const Propositions &postconditions);
    void build_pm_operators(const TaskProxy &task_proxy);

    void postprocess(const TaskProxy &task_proxy);

    void discard_conjunctive_landmarks();

    void approximate_possible_achievers(
        Landmark &landmark, const OperatorsProxy &operators,
        const VariablesProxy &variables) const;
    void calc_achievers(const TaskProxy &task_proxy);

    void add_landmark_node(int set_index, bool goal = false);

    void initialize_hm_table(const VariablesProxy &variables);
    void initialize(const TaskProxy &task_proxy);
    void free_unneeded_memory();

    void print_proposition_set(
        const VariablesProxy &variables,
        const Propositions &propositions) const;
    void print_pm_operator(
        const VariablesProxy &variables, const PiMOperator &op) const;
    void print_conditional_noop(
        const VariablesProxy &variables,
        const ConditionalNoop &conditional_noop,
        std::vector<std::pair<std::set<FactPair>, std::set<FactPair>>>
            &conditions) const;
    std::set<FactPair> print_effect_condition(
        const VariablesProxy &variables,
        const std::vector<int> &effect_condition) const;
    std::set<FactPair> print_conditional_effect(
        const VariablesProxy &variables, const std::vector<int> &effect) const;
    void print_action(
        const VariablesProxy &variables, const PiMOperator &op,
        const std::vector<std::pair<std::set<FactPair>, std::set<FactPair>>>
            &conditions) const;

    void get_m_sets_including_current_var(
        const VariablesProxy &variables, int num_included, int current_var,
        Propositions &current, std::vector<Propositions> &subsets);
    void get_m_sets(
        const VariablesProxy &variables, int num_included, int current_var,
        Propositions &current, std::vector<Propositions> &subsets);

    void get_m_sets_of_set_including_current_proposition(
        const VariablesProxy &variables, int num_included, int current_index,
        Propositions &current, std::vector<Propositions> &subsets,
        const Propositions &superset);
    void get_m_sets_of_set(
        const VariablesProxy &variables, int num_included, int current_index,
        Propositions &current, std::vector<Propositions> &subsets,
        const Propositions &superset);

    void get_split_m_sets_including_current_proposition_from_first(
        const VariablesProxy &variables, int num_included1, int num_included2,
        int current_index1, int current_index2, Propositions &current,
        std::vector<Propositions> &subsets, const Propositions &superset1,
        const Propositions &superset2);
    void get_split_m_sets(
        const VariablesProxy &variables, int num_included1, int num_included2,
        int current_index1, int current_index2, Propositions &current,
        std::vector<Propositions> &subsets, const Propositions &superset1,
        const Propositions &superset2);

    std::vector<Propositions> get_m_sets(const VariablesProxy &variables);

    std::vector<Propositions> get_m_sets(
        const VariablesProxy &variables, const Propositions &superset);

    std::vector<Propositions> get_m_sets(
        const VariablesProxy &variables, const State &state);

    std::vector<Propositions> get_split_m_sets(
        const VariablesProxy &variables, const Propositions &superset1,
        const Propositions &superset2);

    void print_proposition(
        const VariablesProxy &variables, const FactPair &proposition) const;

public:
    LandmarkFactoryHM(
        const std::shared_ptr<AbstractTask> &task, int m,
        bool conjunctive_landmarks, bool use_orders,
        utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
