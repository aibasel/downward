#ifndef MERGE_AND_SHRINK_SHRINK_BISIMULATION_H
#define MERGE_AND_SHRINK_SHRINK_BISIMULATION_H

#include "shrink_strategy.h"

namespace options {
class Options;
}

namespace merge_and_shrink {
struct Signature;

class ShrinkBisimulation : public ShrinkStrategy {
    enum AtLimit {
        RETURN,
        USE_UP
    };

    const bool greedy;
    const AtLimit at_limit;

    void compute_abstraction(const FactoredTransitionSystem &fts,
                             int index,
                             int target_size,
                             StateEquivalenceRelation &equivalence_relation) const;

    int initialize_groups(const FactoredTransitionSystem &fts,
                          int index,
                          std::vector<int> &state_to_group) const;
    void compute_signatures(const FactoredTransitionSystem &fts,
                            int index,
                            std::vector<Signature> &signatures,
                            const std::vector<int> &state_to_group) const;
protected:
    virtual void compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index,
        int target,
        StateEquivalenceRelation &equivalence_relation) const override;
    virtual void dump_strategy_specific_options() const override;
    virtual std::string name() const override;
public:
    explicit ShrinkBisimulation(const options::Options &opts);
    virtual ~ShrinkBisimulation() override;
};
}

#endif
