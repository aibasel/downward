#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include "label.h"

#include "../globals.h"
#include "../operator.h"
#include "../operator_cost.h"

#include <cassert>
#include <vector>

class EquivalenceRelation;
class LabelSignature;

// TODO: possible refactoring: ApproximateLabelReducer and ExactLabelReducer
class LabelReducer {
    int num_labels;
    int num_reduced_labels;

    LabelSignature build_label_signature(const Label &label,
        const std::vector<bool> &var_is_used) const;
public:
    LabelReducer(const std::vector<const Label *> &relevant_labels,
                 const std::vector<int> &abs_vars,
                 std::vector<const Label* > &labels);
    ~LabelReducer();
    void statistics() const;

    // exact label reduction
    LabelReducer(const std::vector<const Label *> &relevant_labels,
                 const EquivalenceRelation *relation,
                 std::vector<const Label* > &labels);
    void statistics2() const;

    int get_no_reduced_labels() const {
        return num_labels - num_reduced_labels;
    }
};

#endif
