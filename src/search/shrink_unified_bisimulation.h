#ifndef SHRINK_UNIFIED_BISIMULATION_H
#define SHRINK_UNIFIED_BISIMULATION_H

#include "shrink_strategy.h"
#include "shrink_bisimulation_base.h"

class Options;

class ShrinkUnifiedBisimulation : public ShrinkBisimulationBase {
    std::vector<int> state_to_group;
    std::vector<bool> group_done;
    std::vector<Signature> signatures;

    std::vector<int> h_to_h_group;
    std::vector<bool> h_group_done;

    int initialize_dfp(const Abstraction &abs);
    int initialize_bisim(const Abstraction &abs);
public:
    ShrinkUnifiedBisimulation(const Options &opts);
    virtual ~ShrinkUnifiedBisimulation();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;
    virtual void shrink(Abstraction &abs, int threshold, bool force = false);

    virtual bool is_bisimulation() const;
    virtual bool has_memory_limit() const;
    virtual bool is_dfp() const;
private:
    void compute_abstraction(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation);
    const bool greedy;
    const bool has_mem_limit;
};

#endif
