#ifndef SHRINK_FH_H
#define SHRINK_FH_H
#include "shrink_strategy.h"
#include <vector>

class Options;

//replaces Shrink_{High,Low}_f_{High,Low}_h
class ShrinkFH : public ShrinkStrategy {
public:
    ShrinkFH(const Options &opts);
    void shrink(Abstraction &abs, bool force, int threshold);

private:
    bool high_f;
    bool high_h;
};


#endif
