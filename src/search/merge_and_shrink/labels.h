#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include "../operator_cost.h"

#include <vector>

class TransitionSystem;
class Label;
class LabelReducer;
class Options;

/*
 The Labels class is basically a container class for the set of all
 labels used by merge-and-shrink transition_systems.
 */
class Labels {
    const LabelReducer *label_reducer;

    std::vector<Label *> labels;
public:
    Labels(const Options &options, OperatorCost cost_type);
    ~Labels();
    void reduce(std::pair<int, int> next_merge,
                const std::vector<TransitionSystem *> &all_transition_systems);
    // TODO: consider removing get_label_by_index and forwarding all required
    // methods of Label and giving access to them by label number.
    const Label *get_label_by_index(int index) const;
    bool is_label_reduced(int label_no) const;
    void dump() const;
    void dump_options() const;

    int get_size() const {
        return labels.size();
    }
};

#endif
