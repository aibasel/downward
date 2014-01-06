#ifndef MERGE_AND_SHRINK_LABEL_REDUCTION_H
#define MERGE_AND_SHRINK_LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

class EquivalenceRelation;
class Label;

/*
 The Labels class is basically a container class for the set of all
 labels used by merge-and-shrink abstractions.
 */
class Labels {
    std::vector<const Label *> labels;
public:
    explicit Labels(OperatorCost cost_type);
    ~Labels();
    // abs_vars is the set of variables of the considered abstraction. The
    // variables in it will *not* be used when checking for local equivalence
    // of labels in relevant_labels
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                       const std::vector<int> &abs_vars,
                       const EquivalenceRelation *relation);
    int get_reduced_label_no(int label_no) const;
    const Label *get_label_by_index(int index) const;
    bool is_label_reduced(int label_no) const;
    void dump() const;

    int get_size() const {
        return labels.size();
    }
};

#endif
