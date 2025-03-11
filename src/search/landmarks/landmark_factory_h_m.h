#ifndef LANDMARKS_LANDMARK_FACTORY_H_M_H
#define LANDMARKS_LANDMARK_FACTORY_H_M_H

#include "landmark_factory.h"

#include <list>
#include <map>

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

/* Corresponds to an operator from the original problem, as well as a
   set of conditional effects that correspond to noops. */
struct PiMOperator {
    std::vector<int> precondition;
    std::vector<int> effect;
    // TODO: Is this still the case?
    // pc separated from effect by a value of -1
    std::vector<std::vector<int>> conditional_noops;
    int index;
};

// represents a proposition in the P^m problem
struct HMEntry {
    // Propositions that belong to this set.
    Propositions propositions;
    // -1 -> current cost infinite
    // 0 -> present in initial state
    int level;

    // TODO: Can we replace the `list` data type?
    std::list<int> landmarks;
    // TODO: What does the following comment mean? What is a "greedy necessary landmark"?
    // Greedy necessary landmarks, disjoint from landmarks
    std::list<int> necessary;

    std::list<int> first_achievers;

    /* TODO: What's the meaning of this? Is it actually using a FactPair to
        represent something completely unrelated?!? */
    /* First int = op index, second int conditional noop effect
       -1 for op itself */
    std::vector<FactPair> pc_for;

    // TODO: Maybe set the propositions in the constructor as well?
    HMEntry()
        : level(-1) {
    }
};

using PropositionSetToIntMap =
    std::map<Propositions, int, PropositionSetComparer>;

class LandmarkFactoryHM : public LandmarkFactory {
    using TriggerSet = std::unordered_map<int, std::set<int>>;

    virtual void generate_landmarks(
        const std::shared_ptr<AbstractTask> &task) override;

    void compute_hm_landmarks(const TaskProxy &task_proxy);
    void compute_noop_landmarks(int op_index, int noop_index,
                                std::list<int> const &local_landmarks,
                                std::list<int> const &local_necessary,
                                int level,
                                TriggerSet &next_trigger);

    void propagate_pm_atoms(int atom_index, bool newly_discovered,
                            TriggerSet &trigger);

    bool possible_noop_set(const VariablesProxy &variables,
                           const Propositions &propositions1,
                           const Propositions &propositions2);
    void build_pm_operators(const TaskProxy &task_proxy);
    // TODO: What is interesting?
    bool interesting(const VariablesProxy &variables,
                     const FactPair &fact1,
                     const FactPair &fact2) const;

    void postprocess(const TaskProxy &task_proxy);

    void discard_conjunctive_landmarks();

    void calc_achievers(const TaskProxy &task_proxy);

    void add_landmark_node(int set_index, bool goal = false);

    void initialize(const TaskProxy &task_proxy);
    void free_unneeded_memory();

    void print_proposition_set(
        const VariablesProxy &variables, const Propositions &fs) const;
    void print_pm_operator(
        const VariablesProxy &variables, const PiMOperator &op) const;

    const int m;
    const bool conjunctive_landmarks;
    const bool use_orders;

    std::unordered_map<int, LandmarkNode *> landmark_node_table;

    std::vector<HMEntry> hm_table;
    std::vector<PiMOperator> pm_operators;
    // Maps each set of <m propositions to an int. TODO: What does this int indicate?
    PropositionSetToIntMap set_indices;
    /*
      The number in the first position represents the amount of unsatisfied
      preconditions of the operator. The vector of numbers in the second
      position represents the amount of unsatisfied preconditions for each
      conditional noop operator.
     */
    std::vector<std::pair<int, std::vector<int>>> unsatisfied_precondition_count;

    void get_m_sets_including_current_var(
        const VariablesProxy &variables, int num_included, int current_var,
        Propositions &current, std::vector<Propositions> &subsets);
    void get_m_sets(
        const VariablesProxy &variables, int num_included, int current_var,
        Propositions &current, std::vector<Propositions> &subsets);

    void get_m_sets_of_set_including_current_proposition(
        const VariablesProxy &variables, int num_included,
        int current_index, Propositions &current,
        std::vector<Propositions> &subsets, const Propositions &superset);
    void get_m_sets_of_set(
        const VariablesProxy &variables, int num_included,
        int current_index, Propositions &current,
        std::vector<Propositions> &subsets, const Propositions &superset);

    void get_split_m_sets_including_current_proposition_from_first(
        const VariablesProxy &variables, int num_included1,
        int num_included2, int current_index1, int current_index2,
        Propositions &current, std::vector<Propositions> &subsets,
        const Propositions &superset1, const Propositions &superset2);
    void get_split_m_sets(
        const VariablesProxy &variables, int num_included1,
        int num_included2, int current_index1, int current_index2,
        Propositions &current, std::vector<Propositions> &subsets,
        const Propositions &superset1, const Propositions &superset2);

    void get_m_sets(const VariablesProxy &variables,
                    std::vector<Propositions> &subsets);

    void get_m_sets(
        const VariablesProxy &variables, std::vector<Propositions> &subsets,
        const Propositions &superset);

    void get_m_sets(const VariablesProxy &variables,
                    std::vector<Propositions> &subsets, const State &state);

    void get_split_m_sets(
        const VariablesProxy &variables, std::vector<Propositions> &subsets,
        const Propositions &superset1, const Propositions &superset2);

    void print_proposition(
        const VariablesProxy &variables, const FactPair &proposition) const;

public:
    LandmarkFactoryHM(int m, bool conjunctive_landmarks,
                      bool use_orders, utils::Verbosity verbosity);

    virtual bool supports_conditional_effects() const override;
};
}

#endif
