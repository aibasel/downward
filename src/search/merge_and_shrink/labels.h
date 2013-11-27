#ifndef LABEL_REDUCTION_H
#define LABEL_REDUCTION_H

#include "../operator_cost.h"

#include <vector>

class LabelReducer;
class Label;

/**
 * @brief The Labels class is basically a container class for the set of all
 *labels used by merge-and-shrink abstractions.
 */
class Labels {
    std::vector<const Label *> labels;
    LabelReducer *label_reducer;
public:
    Labels(OperatorCost cost_type);
    ~Labels();
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                       const std::vector<int> &pruned_vars);
    int get_reduced_label(int label_no) const;
    void free();

    int get_cost_for_label(int label_no) const;
    const Label *get_label_by_index(int index) const;
    void dump() const;

    inline const std::vector<const Label *> &get_labels() const {
        return labels;
    }
};

#endif // LABEL_REDUCTION_H
