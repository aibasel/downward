#ifndef SHRINK_STRATEGY_H
#define SHRINK_STRATEGY_H
#include <string>
#include <vector>

class Abstraction;

typedef int AbstractStateRef;

class ShrinkStrategy {
public:
    ShrinkStrategy();
    virtual ~ShrinkStrategy();
    virtual void shrink(Abstraction &abs, int threshold, bool force = false)=0;
    enum {
        QUITE_A_LOT = 1000000000
    };

    virtual bool has_memory_limit() = 0;
    virtual bool is_bisimulation() = 0;
    virtual bool is_dfp() = 0;

    virtual std::string description() = 0;

};


#endif
