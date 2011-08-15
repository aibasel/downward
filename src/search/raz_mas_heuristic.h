#ifndef MAS_HEURISTIC_H
#define MAS_HEURISTIC_H

#include "heuristic.h"
#include "shrink_strategy.h"

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
    MAX_MERGE_STRATEGY
};

class MergeAndShrinkHeuristic : public Heuristic {
    const int max_abstract_states;
    const int max_abstract_states_before_merge;
    const int abstraction_count;
    const MergeStrategy merge_strategy;
    const ShrinkStrategy *shrink_strategy;
    const bool use_label_simplification;
    const bool use_expensive_statistics;

    std::vector<Abstraction *> abstractions;
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
    MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic();
};

#endif
