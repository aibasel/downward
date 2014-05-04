#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include "../operator_cost.h"

#include <vector>

class Abstraction;
class Label;
class LabelReducer;
class Options;

/*
 The Labels class is basically a container class for the set of all
 labels used by merge-and-shrink abstractions.
 */
class Labels {
    const bool unit_cost;
    const LabelReducer *label_reducer;

    std::vector<Label *> labels;
public:
    Labels(bool unit_cost, const Options &options, OperatorCost cost_type);
    ~Labels();
    void reduce(std::pair<int, int> next_merge,
                const std::vector<Abstraction *> &all_abstractions);
    // TODO: consider removing get_label_by_index and forwarding all required
    // methods of Label and giving access to them by label number.
    const Label *get_label_by_index(int index) const;
    bool is_label_reduced(int label_no) const;
    void dump() const;
    void dump_options() const;

    int get_size() const {
        return labels.size();
    }
    bool is_unit_cost() const {
        return unit_cost;
    }
};

#endif
