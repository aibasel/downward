#include "raz_abstraction.h"
#include "shrink_bisimulation_base.h"
#include <cassert>
#include <iostream>
#include <vector>

ShrinkBisimulationBase::ShrinkBisimulationBase() {
}

ShrinkBisimulationBase::~ShrinkBisimulationBase() {
}


bool ShrinkBisimulationBase::are_bisimilar(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2,
    bool greedy_bisim,
    const vector<int> &group_to_h, int source_h_1, int source_h_2) const {

    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            bool relevant = !greedy_bisim
                            || group_to_h[succ_sig1[trans1].second] < source_h_1;
            if (relevant && ::find(succ_sig2.begin(), succ_sig2.end(),
                                   succ_sig1[trans1]) == succ_sig2.end()) {
                bool found_equivalent_trans = false;
                for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
                    if (succ_sig1[trans1] == succ_sig2[trans2]) {
                        found_equivalent_trans = true;
                        break;
                    }
                }
                if (!found_equivalent_trans)
                    return false;
            }
        }
        for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
            bool relevant = !greedy_bisim
                            || group_to_h[succ_sig2[trans2].second] < source_h_2;
            if (relevant && ::find(succ_sig1.begin(), succ_sig1.end(),
                                   succ_sig2[trans2]) == succ_sig1.end()) {
                bool found_equivalent_trans = false;
                for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
                    if (succ_sig1[trans1] == succ_sig2[trans2]) {
                        found_equivalent_trans = true;
                        break;
                    }
                }
                if (!found_equivalent_trans)
                    return false;
            }
        }
    }
    return true;
}
