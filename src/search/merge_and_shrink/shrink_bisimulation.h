#ifndef MERGE_AND_SHRINK_SHRINK_BISIMULATION_H
#define MERGE_AND_SHRINK_SHRINK_BISIMULATION_H

#include "shrink_strategy.h"

class Options;
struct Signature;

class ShrinkBisimulation : public ShrinkStrategy {
    enum AtLimit {
        RETURN,
        USE_UP
    };

    const bool greedy;
    const AtLimit at_limit;

    void compute_abstraction(const TransitionSystem &ts,
                             int target_size,
                             StateEquivalenceRelation &equivalence_relation);

    int initialize_groups(const TransitionSystem &ts,
                          std::vector<int> &state_to_group);
    void compute_signatures(const TransitionSystem &ts,
                            std::vector<Signature> &signatures,
                            const std::vector<int> &state_to_group);
protected:
    virtual void shrink(const TransitionSystem &ts,
                        int target,
                        StateEquivalenceRelation &equivalence_relation);
    virtual void dump_strategy_specific_options() const;
    virtual std::string name() const;
public:
    explicit ShrinkBisimulation(const Options &opts);
    virtual ~ShrinkBisimulation();
};

#endif
