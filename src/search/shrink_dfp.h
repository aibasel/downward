#ifndef SHRINK_DFP_H
#define SHRINK_DFP_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkDFP : public ShrinkStrategy {
public:
    ShrinkDFP();
    void shrink(Abstraction &abs, bool force, int threshold);
private:
    void compute_abstraction_dfp_action_cost_support(
        Abstraction &abs, 
        int target_size,
        vector<slist<AbstractStateRef> > &collapsed_groups,
        bool enable_greedy_bisimulation);
};


#endif
