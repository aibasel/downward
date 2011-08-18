#ifndef SHRINK_BISIMULATION_H
#define SHRINK_BISIMULATION_H
#include "shrink_strategy.h"
#include "shrink_bisimulation_base.h"
#include <vector>

class Options;

class ShrinkBisimulation : public ShrinkBisimulationBase {
public:
    ShrinkBisimulation(const Options &opts);
    ShrinkBisimulation(bool greedy, bool memory_limit);
    virtual ~ShrinkBisimulation();

    virtual void shrink(Abstraction &abs, int threshold, bool force = false);

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
