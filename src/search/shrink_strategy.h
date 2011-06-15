#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <vector>

class Abstraction;

class ShrinkStrategy {
public:
    virtual void shrink(Abstraction &abs, bool force, int threshold)=0;
};


#endif
