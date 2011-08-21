#ifndef SHRINK_BISIMULATION_H
#define SHRINK_BISIMULATION_H
#include "shrink_strategy.h"
#include "shrink_bisimulation_base.h"
#include <vector>

class Options;

class ShrinkBisimulation : public ShrinkBisimulationBase {
public:
    ShrinkBisimulation(const Options &opts);
    virtual ~ShrinkBisimulation();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;
    virtual void shrink(Abstraction &abs, int threshold, bool force = false);
    virtual void shrink_atomic(Abstraction &abs);
    virtual void shrink_before_merge(Abstraction &abs1, Abstraction &abs2);
private:
    void compute_abstraction(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation) const;
    const bool greedy;
    const bool has_mem_limit;
};


#endif
