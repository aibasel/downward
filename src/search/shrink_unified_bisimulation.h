#ifndef SHRINK_UNIFIED_BISIMULATION_H
#define SHRINK_UNIFIED_BISIMULATION_H

// TODO: Once we know we don't need the old classes any more,
//       rename this to shrink_bisimulation.h, also renaming the class
//       and command-line option.

#include "shrink_strategy.h"

class Options;
class Signature;

class ShrinkUnifiedBisimulation : public ShrinkStrategy {
    /*
      greedy: Use greedy bisimulation rather than exact bisimulation.

      threshold: Shrink the abstraction iff it is larger than this
      size. Note that this is set independently from max_states, which
      is the number of states to which the abstraction is shrunk.
    */

    const bool greedy;
    const int threshold;
    const bool skip_atomic_bisimulation;
    const bool initialize_by_h;
    const bool group_by_h;

    std::vector<int> state_to_group;
    std::vector<bool> group_done;
    std::vector<Signature> signatures;

    std::vector<int> h_to_h_group;
    std::vector<bool> h_group_done;

    void compute_abstraction(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation);

    int initialize_dfp(const Abstraction &abs);
    int initialize_bisim(const Abstraction &abs);
public:
    ShrinkUnifiedBisimulation(const Options &opts);
    virtual ~ShrinkUnifiedBisimulation();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;

    virtual WhenToNormalize when_to_normalize(bool use_label_reduction) const;

    virtual void shrink(Abstraction &abs, int target, bool force = false);
    virtual void shrink_atomic(Abstraction &abs);
    virtual void shrink_before_merge(Abstraction &abs1, Abstraction &abs2);

    static ShrinkStrategy *create_default();
};

// TODO: Move the following two classes into the CC file once we have
//       got rid of the legacy bisimulation classes (ShrinkDFP and
//       ShrinkBisimulation).

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
