#include "shrink_bisimulation_base.h"

#include "option_parser.h"  // TODO Should be removable later.
#include "raz_abstraction.h"
#include "shrink_unified_bisimulation.h"
// TODO: Get rid of this latter include, which is only needed for
//       our definition of shrink_atomic below.

#include <cassert>
#include <limits>
#include <vector>
using namespace std;


static const int infinity = numeric_limits<int>::max();


ShrinkBisimulationBase::ShrinkBisimulationBase(const Options &opts)
    : ShrinkStrategy(opts) {
}

ShrinkBisimulationBase::~ShrinkBisimulationBase() {
}

ShrinkStrategy::WhenToNormalize ShrinkBisimulationBase::when_to_normalize(
    bool use_label_reduction) const {
    if (use_label_reduction)
        return AFTER_MERGE;
    else
        return BEFORE_MERGE;
}
void ShrinkBisimulationBase::shrink_atomic(Abstraction &abs) {
    // Perform an exact bisimulation on all atomic abstractions.

     // TODO/HACK: Come up with a better way to do this than generating
    // a new shrinking class instance in this roundabout fashion. We
    // shouldn't need to generate a new instance at all.

    if (is_dfp()) {
        // We don't bisimulate here because the old code didn't, also
        // that was most probably an accident. TODO: Investigate the
        // effect of bisimulating here. The reason why we didn't just
        // add this is that it actually hurt performance on one of our
        // test cases, Sokoban-Opt-#12 with DFP-gop-200K (as well as
        // other DFP-based strategies). This may well be a random
        // mishap, but still it's certainly better to be careful here.
        cout << "DEBUG: I am DFP and I don't pre-bisimulate atomic abstractions." << endl;
        return;
    }

    int old_size = abs.size();
    Options opts;
    opts.set("max_states", infinity);
    opts.set("max_states_before_merge", infinity);
    opts.set("greedy", false);
    opts.set("memory_limit", false);
    ShrinkUnifiedBisimulation(opts).shrink(abs, abs.size(), true);
    if (abs.size() != old_size) {
        cout << "Atomic abstraction simplified "
             << "from " << old_size
             << " to " << abs.size()
             << " states." << endl;
    }
}

// is_sorted is only needed for debugging purposes.
template<class T>
bool is_sorted(const vector<T> &vec) {
    for (size_t i = 1; i < vec.size(); ++i)
        if (vec[i] < vec[i - 1])
            return false;
    return true;
}

bool ShrinkBisimulationBase::are_bisimilar(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2,
    bool greedy_bisim,
    const vector<int> &group_to_h, int source_h_1, int source_h_2) const {

    assert(is_sorted(succ_sig1));
    assert(is_sorted(succ_sig2));

    if (!greedy_bisim)
        return succ_sig1 == succ_sig2;

    if (source_h_1 != source_h_2) // TODO: If that is indeed always true, should switch to one h arg.
        abort();
    int h = source_h_1;

    /* Check that succ_sig1 and succ_sig2 are identical if we ignore
       arcs that leads to nodes with higher h value. The algorithm is
       a parallel scan through the two signatures similar to the merge
       step of merge sort. */
    SuccessorSignature::const_iterator iter1 = succ_sig1.begin();
    SuccessorSignature::const_iterator iter2 = succ_sig2.begin();
    SuccessorSignature::const_iterator end1 = succ_sig1.end();
    SuccessorSignature::const_iterator end2 = succ_sig2.end();
    while (iter1 != end1 && iter2 != end2) {
        if (*iter1 == *iter2) {
            ++iter1;
            ++iter2;
        } else if(*iter1 < *iter2) {
            if (group_to_h[iter1->second] < h)
                return false;
            ++iter1;
        } else {
            if (group_to_h[iter2->second] < h)
                return false;
            ++iter2;
        }
    }
    if (iter1 == end1) {
        while (iter2 != end2) {
            if (group_to_h[iter2->second] < h)
                return false;
            ++iter2;
        }
    } else {
        while (iter1 != end1) {
            if (group_to_h[iter1->second] < h)
                return false;
            ++iter1;
        }
    }
    return true;
}

