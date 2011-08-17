#include "raz_abstraction.h"
#include "shrink_bisimulation_base.h"
#include <cassert>
#include <iostream>
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

    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            // TODO: Shouldn't the next "<" be "<="?
            bool relevant = group_to_h[succ_sig1[trans1].second] < source_h_1;
            if (relevant && ::find(succ_sig2.begin(), succ_sig2.end(),
                                   succ_sig1[trans1]) == succ_sig2.end())
                return false;
        }
        for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
            // TODO: Shouldn't the next "<" be "<="?
            bool relevant = group_to_h[succ_sig2[trans2].second] < source_h_2;
            if (relevant && ::find(succ_sig1.begin(), succ_sig1.end(),
                                   succ_sig2[trans2]) == succ_sig1.end())
                return false;
        }
    }
    return true;
}
