#ifndef MERGE_AND_SHRINK_LABEL_REDUCTION_H
#define MERGE_AND_SHRINK_LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

class Abstraction;
class Label;

enum LabelReduction {
    NONE,
    APPROXIMATIVE,
    APPROXIMATIVE_WITH_FIXPOINT,
    EXACT,
    EXACT_WITH_FIXPOINT
};

/*
 The Labels class is basically a container class for the set of all
 labels used by merge-and-shrink abstractions.
 */
class Labels {
    std::vector<const Label *> labels;
    LabelReduction label_reduction;
    bool exact;
    bool fixpoint;
public:
    Labels(OperatorCost cost_type, LabelReduction label_reduction);
    ~Labels();
    int reduce(int abs_index,
               const std::vector<Abstraction *> &all_abstractions);
    int get_reduced_label_no(int label_no) const;
    const Label *get_label_by_index(int index) const;
    // TODO: rename and/or add is_leaf method
    bool is_label_reduced(int label_no) const;
    void dump() const;
    void dump_options() const;

    int get_size() const {
        return labels.size();
    }
};

#endif
