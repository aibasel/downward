#ifndef SHRINK_BISIMULATION_H
#define SHRINK_BISIMULATION_H
#include "shrink_strategy.h"
#include <vector>

class Options;

class ShrinkBisimulation : public ShrinkStrategy {
public:
    ShrinkBisimulation(const Options &opts);
    ShrinkBisimulation(bool greedy, bool memory_limit);
    ~ShrinkBisimulation();
    void shrink(Abstraction &abs, int threshold, bool force = false);

    bool is_bisimulation();
    bool has_memory_limit();
    bool is_dfp();

    std::string description();
private:
    void compute_abstraction(
        Abstraction &abs,
        int target_size, 
        vector<slist<AbstractStateRef> > &collapsed_groups) const;
    bool greedy;
    bool has_mem_limit;
};


#endif
