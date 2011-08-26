#ifndef SHRINK_DFP_H
#define SHRINK_DFP_H
#include "shrink_bisimulation_base.h"
#include "shrink_strategy.h"
#include <vector>

class Options;

enum DFPStyle {DEFAULT, ENABLE_GREEDY_BISIMULATION};

class ShrinkDFP : public ShrinkBisimulationBase {
public:
    ShrinkDFP(const Options &opts);
    virtual ~ShrinkDFP();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;
    virtual void shrink(Abstraction &abs, int threshold, bool force = false);
    virtual void shrink_atomic(Abstraction &abs);
private:
    void compute_abstraction_dfp_action_cost_support(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation,
        bool enable_greedy_bisimulation) const;

    const DFPStyle dfp_style;
};


#endif
