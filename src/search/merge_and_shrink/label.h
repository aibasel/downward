#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

#include "../operator.h"

#include <vector>

class Label {
    int index;
    int cost;
    const std::vector<Prevail> prevail;
    const std::vector<PrePost> pre_post;

    // HACK! because label_reducer uses const Label pointers, we must declare
    // add_mapping_label as const to be allowed to add mapping labels after
    // the creation of the (const) label
    mutable std::vector<const Label *> mapping_labels;
public:
    Label(int index, int cost, const std::vector<Prevail> &prevail, const std::vector<PrePost> &pre_post);
    Label(int index, const Label *label);
    void add_mapping_label(const Label *label) const;
    const std::vector<Prevail> &get_prevail() const {return prevail; }
    const std::vector<PrePost> &get_pre_post() const {return pre_post; }
    int get_index() const {
        return index;
    }
    int get_cost() const {
        return cost;
    }
    void dump() const;

    mutable bool marker1, marker2; // HACK! HACK!
};

#endif
