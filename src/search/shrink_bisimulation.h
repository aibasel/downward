#ifndef SHRINK_BISIMULATION_H
#define SHRINK_BISIMULATION_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkBisimulation : public ShrinkStrategy {
public:
    ShrinkBisimulation(...);
    void shrink(Abstraction &abs, bool force, int threshold);
};


#endif
