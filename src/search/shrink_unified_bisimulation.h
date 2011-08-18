#ifndef SHRINK_UNIFIED_BISIMULATION_H
#define SHRINK_UNIFIED_BISIMULATION_H

#include "shrink_strategy.h"
#include "shrink_bisimulation_base.h"

class Options;

class ShrinkUnifiedBisimulation : public ShrinkBisimulationBase {
public:
    ShrinkUnifiedBisimulation(const Options &opts);
    ShrinkUnifiedBisimulation(bool greedy, bool memory_limit);
    virtual ~ShrinkUnifiedBisimulation();

    virtual void shrink(Abstraction &abs, int threshold, bool force = false) const;

    virtual bool is_bisimulation() const;
    virtual bool has_memory_limit() const;
    virtual bool is_dfp() const;

    virtual std::string description() const;
private:
    void compute_abstraction(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation) const;
    const bool greedy;
    const bool has_mem_limit;
};

#endif
