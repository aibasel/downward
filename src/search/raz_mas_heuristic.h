#ifndef MAS_HEURISTIC_H
#define MAS_HEURISTIC_H

#include "heuristic.h"

#include <utility>
#include <vector>

class Abstraction;

enum MergeStrategy {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_DFP,
    MERGE_LINEAR_LEVEL,
    MERGE_LINEAR_REVERSE_LEVEL,
    MERGE_LEVEL_THEN_INVERSE,
    MERGE_INVERSE_THEN_LEVEL,
    MAX_MERGE_STRATEGY
};

enum ShrinkStrategy {
    SHRINK_HIGH_F_LOW_H,
    SHRINK_LOW_F_LOW_H,
    SHRINK_HIGH_F_HIGH_H,
    SHRINK_RANDOM,
    SHRINK_DFP,
    SHRINK_BISIMULATION,
    SHRINK_BISIMULATION_NO_MEMORY_LIMIT,
    SHRINK_DFP_ENABLE_GREEDY_BISIMULATION,
    SHRINK_DFP_ENABLE_FURTHER_LABEL_REDUCTION,
    SHRINK_DFP_ENABLE_GREEDY_THEN_LABEL_REDUCTION,
    SHRINK_DFP_ENABLE_LABEL_REDUCTION_THEN_GREEDY,
    SHRINK_DFP_ENABLE_LABEL_REDUCTION_AND_GREEDY_CHOOSE_MAX,
    SHRINK_GREEDY_BISIMULATION_NO_MEMORY_LIMIT,
    SHRINK_BISIMULATION_REDUCING_ALL_LABELS_NO_MEMORY_LIMIT,
    SHRINK_GREEDY_BISIMULATION_REDUCING_ALL_LABELS_NO_MEMORY_LIMIT,
    MAX_SHRINK_STRATEGY
};

class MergeAndShrinkHeuristic : public Heuristic {
    int max_abstract_states;                                    //TODO - removed const for raz's double abstraction experiment
    int max_abstract_states_before_merge;       //TODO - removed const for raz's double abstraction experiment
    const int abstraction_count;
    MergeStrategy merge_strategy;                       //TODO - removed const for raz's double abstraction experiment
    ShrinkStrategy shrink_strategy;                     //TODO - removed const for raz's double abstraction experiment
    const bool use_label_simplification;
    const bool use_expensive_statistics;
    const double merge_mixing_parameter;

    std::vector<Abstraction *> abstractions;
    void verify_no_axioms_no_cond_effects() const;
    std::pair<int, int> compute_shrink_sizes(int size1, int size2) const;
    Abstraction *build_abstraction(bool is_first = true);

    void dump_options() const;
    void warn_on_unusual_options() const;

    //Gives an approximation on the number of extra transitions added to the composed system,
    //composed of abstraction and the relevant atomic abstractions, if relevant_ops are reduced.
    int approximate_added_transitions(
        const std::vector<int> &transitions_to_reduce, const std::vector<
            int> &relevant_atomic_abs, const Abstraction *abstraction,
        const std::vector<Abstraction *> &atomic_abstractions) const;

    void get_ops_to_reduce(Abstraction *abs1, Abstraction *abs2, std::vector<
                               std::vector<const Operator *> > &result, int reduce_strategy);
    int get_distance_from_op(const Operator *op, const Operator *other_op);

protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    MergeAndShrinkHeuristic(int max_abstract_states,
                            int max_abstract_states_before_merge, int abstraction_count,
                            MergeStrategy merge_strategy, ShrinkStrategy shrink_strategy,
                            bool use_label_simplification, bool use_expensive_statistics,
                            double merge_mixing_parameter);
    ~MergeAndShrinkHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
