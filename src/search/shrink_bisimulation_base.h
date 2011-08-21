#ifndef SHRINK_BISIMULATION_BASE_H
#define SHRINK_BISIMULATION_BASE_H
#include "shrink_strategy.h"
#include <vector>

/* A base class for shrink strategies that create a bisimulation
   (ShrinkBisimulation and ShrinkDFP).
*/

class Options;

class ShrinkBisimulationBase : public ShrinkStrategy {
protected:
    bool are_bisimilar(
        const std::vector<std::pair<int, int> > &succ_sig1,
        const std::vector<std::pair<int, int> > &succ_sig2,
        bool greedy_bisim,
        const std::vector<int> &group_to_h,
        int source_h_1, int source_h_2) const;
public:
    ShrinkBisimulationBase(const Options &opt);
    virtual ~ShrinkBisimulationBase();

    virtual WhenToNormalize when_to_normalize(bool use_label_reduction) const;
    virtual void shrink_atomic(Abstraction &abs);
};

// TODO: document purpose of Signatures
typedef std::vector<std::pair<int, int> > SuccessorSignature;

struct Signature {
    int h;
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool matches(const Signature &other) const {
        return h == other.h && group == other.group && succ_signature
               == other.succ_signature;
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }
};

#endif
