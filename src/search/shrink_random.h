#ifndef SHRINK_RANDOM_H
#define SHRINK_RANDOM_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkRandom : public ShrinkStrategy {
public:
    ShrinkRandom(...);
    void shrink(Abstraction &abs, bool force, int threshold);
};


#endif
