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
};

/* A successor signature characterizes the behaviour of an abstract
   state in so far as bisimulation cares about it. States with
   identical successor signature are not distinguished by
   bisimulation.

   Each entry in the vector is a pair of (label, equivalence class of
   successor). The bisimulation algorithm requires that the vector is
   sorted and uniquified. */

typedef std::vector<std::pair<int, int> > SuccessorSignature;

/* TODO: The following class should probably be renamed. It encodes
   all we need to know about a state for bisimulation: its h value,
   which equivalence class ("group") it currently belongs to, its
   signatures (see above), and what the original state is. */

struct Signature {
    int h;
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
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
