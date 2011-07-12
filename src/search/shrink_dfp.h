#ifndef SHRINK_DFP_H
#define SHRINK_DFP_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkDFP : public ShrinkStrategy {
public:
    ShrinkDFP();
    void shrink(Abstraction &abs, int threshold, bool force);

    bool is_bisimulation();
    bool has_memory_limit();
    bool is_dfp();

private:
    void compute_abstraction_dfp_action_cost_support(
        Abstraction &abs, 
        int target_size,
        vector<slist<AbstractStateRef> > &collapsed_groups,
        bool enable_greedy_bisimulation);

    bool is_bisim;
};


#endif
