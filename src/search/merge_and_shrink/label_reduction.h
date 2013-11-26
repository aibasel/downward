#ifndef LABEL_REDUCTION_H
#define LABEL_REDUCTION_H

#include "../option_parser.h"

#include <vector>

class LabelReducer;
class Label;

class LabelReduction {
    const Options options;
    LabelReducer *label_reducer;
    int next_label_no;

    std::vector<const Label *> labels;
public:
    LabelReduction(const Options &options);
    void reduce_labels(const std::vector<const Label *> &relevant_labels,
                  const std::vector<int> &pruned_vars);

    int get_reduced_label(int label_no) const;
    void free();
    int get_cost_for_label(int label_no) const;
    const Label *get_label_by_index(int index) const;
    const std::vector<const Label *> &get_labels() const {
        return labels;
    }
};

#endif // LABEL_REDUCTION_H
