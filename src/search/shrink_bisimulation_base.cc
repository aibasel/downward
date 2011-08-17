#include "shrink_bisimulation_base.h"

#include "raz_abstraction.h"

#include <cassert>
#include <vector>

ShrinkBisimulationBase::ShrinkBisimulationBase() {
}

ShrinkBisimulationBase::~ShrinkBisimulationBase() {
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

    SuccessorSignature::const_iterator iter1 = succ_sig1.begin();
    SuccessorSignature::const_iterator iter2 = succ_sig2.begin();
    SuccessorSignature::const_iterator end1 = succ_sig1.end();
    SuccessorSignature::const_iterator end2 = succ_sig2.end();

/*
  // TODO: This simpler variant somehow did not seem to work, or at least took forever. Why?

    for (;;) {
        while (iter1 != end1 && group_to_h[iter1->second] < h)
            ++iter1;
        while (iter2 != end2 && group_to_h[iter2->second] < h)
            ++iter2;
        bool at_end1 = (iter1 == end1);
        bool at_end2 = (iter2 == end2);
        if (at_end1 && at_end2)
            return true;
        if (at_end1 || at_end2 || *iter1 != *iter2)
            return false;
        ++iter1;
        ++iter2;
    }
*/
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

