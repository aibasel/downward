#ifndef MERGE_AND_SHRINK_LABEL_REDUCTION_H
#define MERGE_AND_SHRINK_LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

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
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                       const std::vector<int> &abs_vars);
    int get_reduced_label_no(int label_no) const;
    const Label *get_label_by_index(int index) const;
    void dump() const;

    int get_size() const {
        return labels.size();
    }
};

#endif
