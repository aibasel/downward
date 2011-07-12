#ifndef SHRINK_BISIMULATION_H
#define SHRINK_BISIMULATION_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkBisimulation : public ShrinkStrategy {
public:
    ShrinkBisimulation(bool greedy, bool memory_limit);
    void shrink(Abstraction &abs, int threshold, bool force);

    bool is_bisimulation();
    bool has_memory_limit();
    bool is_dfp();

private:
    void compute_abstraction_action_cost_support(
        Abstraction &abs,
        int target_size, vector<slist<AbstractStateRef> > &collapsed_groups,
        bool enable_greedy_bisimulation) const;
    bool has_mem_limit;
    bool greedy;

};


#endif
