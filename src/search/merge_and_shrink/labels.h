#ifndef LABEL_REDUCTION_H
#define LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

class LabelReducer;
class Label;

/*
 The Labels class is basically a container class for the set of all
 labels used by merge-and-shrink abstractions.
 */
class Labels {
    // the list of *all* labels ever created
    std::vector<const Label *> labels;
    // the list of *active* labels, accessed by index
    std::vector<const Label *> reduced_label_by_index;
    LabelReducer *label_reducer;
public:
    explicit Labels(OperatorCost cost_type);
    ~Labels();
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                       const std::vector<int> &pruned_vars);
    int get_reduced_label_no(int label_no) const;
    const Label *get_reduced_label(const Label *label) const;
    void free();

    const Label *get_label_by_index(int index) const;
    void dump() const;

    std::size_t get_size() const {
        return labels.size();
    }
    const std::vector<const Label *> &get_labels() const {
        return labels;
    }
    bool are_labels_reduced() const {
        return label_reducer;
    }
};

#endif // LABEL_REDUCTION_H


/*void OperatorLabel::update_root(Label *new_root) {
    root = new_root;
}

void CompositeLabel::update_root(Label *new_root) {
    for (size_t i = 0; i < parents.size(); ++i)
        parents[i]->update_root(new_root);
    root = new_root;
}*/
