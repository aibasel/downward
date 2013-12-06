#ifndef MERGE_AND_SHRINK_LABEL_REDUCTION_H
#define MERGE_AND_SHRINK_LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

class LabelReducer;
class Label;

/**
 * @brief The Labels class is basically a container class for the set of all
 *labels used by merge-and-shrink abstractions.
 */
class Labels {
    // the list of *all* labels ever created
    std::vector<const Label *> labels;
    // the list of *active* labels, accessed by index
    std::vector<const Label *> reduced_label_by_index;
    LabelReducer *label_reducer;
public:
    Labels(OperatorCost cost_type);
    ~Labels();
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                       const std::vector<int> &pruned_vars);
    int get_reduced_label_no(int label_no) const;
    const Label *get_reduced_label(const Label *label) const;
    void free();

    const Label *get_label_by_index(int index) const;
    void dump() const;

    inline unsigned int get_size() const {
        return labels.size();
    }
    inline const std::vector<const Label *> &get_labels() const {
        return labels;
    }
    inline bool are_labels_reduced() const {
        return label_reducer;
    }
};

#endif
