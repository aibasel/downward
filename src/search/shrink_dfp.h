#ifndef SHRINK_DFP_H
#define SHRINK_DFP_H
#include "shrink_strategy.h"
#include <vector>

class ShrinkDFP : public ShrinkStrategy {
public:
    ShrinkDFP(...);
    void shrink(Abstraction &abs, bool force, int threshold);
};


#endif
