#ifndef SHRINK_DFP_H
#define SHRINK_DFP_H
#include "shrink_strategy.h"
#include <vector>

class Options;

enum DFPStyle {DEFAULT, ENABLE_GREEDY_BISIMULATION};

class ShrinkDFP : public ShrinkStrategy {
public:
    ShrinkDFP(const Options &opts);
    virtual ~ShrinkDFP();
    virtual void shrink(Abstraction &abs, int threshold, bool force = false) const;

    virtual bool is_bisimulation() const;
    virtual bool has_memory_limit() const;
    virtual bool is_dfp() const;

    virtual std::string description() const;
private:
    void compute_abstraction_dfp_action_cost_support(
        Abstraction &abs, 
        int target_size,
        vector<slist<AbstractStateRef> > &collapsed_groups,
        bool enable_greedy_bisimulation) const;

    DFPStyle dfp_style;
};


#endif
