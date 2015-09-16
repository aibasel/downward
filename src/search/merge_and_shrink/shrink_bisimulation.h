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
                             StateEquivalenceRelation &equivalence_relation) const;

    int initialize_groups(const TransitionSystem &ts,
                          std::vector<int> &state_to_group) const;
    void compute_signatures(const TransitionSystem &ts,
                            std::vector<Signature> &signatures,
                            const std::vector<int> &state_to_group) const;
protected:
    virtual void compute_equivalence_relation(
        const TransitionSystem &ts,
        int target,
        StateEquivalenceRelation &equivalence_relation) const override;
    virtual void dump_strategy_specific_options() const override;
    virtual std::string name() const override;
public:
    explicit ShrinkBisimulation(const Options &opts);
    virtual ~ShrinkBisimulation();
};

#endif
