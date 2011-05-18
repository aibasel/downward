#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include "raz_abstraction.h"
#include <vector>

class ShrinkStrategy {
public:
    virtual void shrink(Abstraction &abs, bool force, int threshold)=0;
};


#endif
