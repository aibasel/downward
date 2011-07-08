#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <vector>

class Abstraction;

typedef int AbstractStateRef;

class ShrinkStrategy {
public:
    virtual void shrink(Abstraction &abs, bool force, int threshold)=0;
    enum {
        QUITE_A_LOT = 1000000000
    };

};


#endif
