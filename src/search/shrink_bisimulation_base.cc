#include "raz_abstraction.h"
#include "shrink_bisimulation_base.h"
#include <cassert>
#include <iostream>
#include <vector>

ShrinkBisimulationBase::ShrinkBisimulationBase() {
}

ShrinkBisimulationBase::~ShrinkBisimulationBase() {
}


//Given two successive signatures and the pairs of labels that should be reduced, returns true if the source states are bisimilar (after reducing the labels)
bool ShrinkBisimulationBase::are_bisimilar_wrt_label_reduction(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2,
    const vector<pair<int, int> > &pairs_of_labels_to_reduce) const {
    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            if (::find(succ_sig2.begin(), succ_sig2.end(), succ_sig1[trans1])
                == succ_sig2.end()) {
                int op_no = succ_sig1[trans1].first;
                int target = succ_sig1[trans1].second;
                bool found_reduced_trans = false;
                for (int pair = 0; pair < pairs_of_labels_to_reduce.size(); pair++) {
                    ::pair<int, int> pair_to_find = make_pair(-1, -1);
                    if (pairs_of_labels_to_reduce[pair].first == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].second;
                    else if (pairs_of_labels_to_reduce[pair].second == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].first;
                    if (pair_to_find.first != -1) {
                        pair_to_find.second = target;
                        if (::find(succ_sig2.begin(), succ_sig2.end(),
                                   pair_to_find) != succ_sig2.end()) {
                            found_reduced_trans = true;
                            break;
                        }
                    }
                }
                if (!found_reduced_trans)
                    return false;
            }
        }

        for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
            if (::find(succ_sig1.begin(), succ_sig1.end(), succ_sig2[trans2])
                == succ_sig1.end()) {
                int op_no = succ_sig2[trans2].first;
                int target = succ_sig2[trans2].second;
                bool found_reduced_trans = false;
                for (int pair = 0; pair < pairs_of_labels_to_reduce.size(); pair++) {
                    ::pair<int, int> pair_to_find = make_pair(-1, -1);
                    if (pairs_of_labels_to_reduce[pair].first == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].second;
                    else if (pairs_of_labels_to_reduce[pair].second == op_no)
                        pair_to_find.first =
                            pairs_of_labels_to_reduce[pair].first;
                    if (pair_to_find.first != -1) {
                        pair_to_find.second = target;
                        if (::find(succ_sig1.begin(), succ_sig1.end(),
                                   pair_to_find) != succ_sig1.end()) {
                            found_reduced_trans = true;
                            break;
                        }
                    }
                }
                if (!found_reduced_trans)
                    return false;
            }
        }
    }
    return true;
}

bool ShrinkBisimulationBase::are_bisimilar(
    const vector<pair<int, int> > &succ_sig1,
    const vector<pair<int, int> > &succ_sig2, bool ignore_all_labels,
    bool greedy_bisim, bool further_label_reduction,
    const vector<int> &group_to_h, int source_h_1, int source_h_2,
    const vector<pair<int, int> > &pairs_of_labels_to_reduce) const {
    if (further_label_reduction)
        return are_bisimilar_wrt_label_reduction(succ_sig1, succ_sig2,
                                                 pairs_of_labels_to_reduce);
    if (succ_sig1 != succ_sig2) {
        for (int trans1 = 0; trans1 < succ_sig1.size(); trans1++) {
            bool relevant = !greedy_bisim
                            || group_to_h[succ_sig1[trans1].second] < source_h_1;
            if (relevant && ::find(succ_sig2.begin(), succ_sig2.end(),
                                   succ_sig1[trans1]) == succ_sig2.end()) {
                bool found_equivalent_trans = false;
                for (int trans2 = 0; trans2 < succ_sig2.size(); trans2++) {
                    if ((ignore_all_labels && succ_sig1[trans1].second
                         == succ_sig2[trans2].second) || (!ignore_all_labels
                                                          && succ_sig1[trans1] == succ_sig2[trans2])) {
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
                    if ((ignore_all_labels && succ_sig1[trans1].second
                         == succ_sig2[trans2].second) || (!ignore_all_labels
                                                          && succ_sig1[trans1] == succ_sig2[trans2])) {
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
