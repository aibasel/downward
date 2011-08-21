#ifndef SHRINK_BISIMULATION_BASE_H
#define SHRINK_BISIMULATION_BASE_H
#include "shrink_strategy.h"
#include <vector>

/* A base class for shrink strategies that create a bisimulation
   (ShrinkBisimulation and ShrinkDFP).
*/

class Options;
typedef std::vector<std::pair<int, int> > SuccessorSignature;

class ShrinkBisimulationBase : public ShrinkStrategy {
protected:
    bool are_bisimilar(
        const SuccessorSignature &succ_sig1,
        const SuccessorSignature &succ_sig2,
        bool greedy_bisim,
        const std::vector<int> &group_to_h,
        int source_h_1, int source_h_2) const;
public:
    ShrinkBisimulationBase(const Options &opt);
    virtual ~ShrinkBisimulationBase();

    virtual WhenToNormalize when_to_normalize(bool use_label_reduction) const;
};

#endif
