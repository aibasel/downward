#ifndef LABEL_H
#define LABEL_H

#include "../operator.h"

#include <vector>

//class Operator;

class Label {
    int index;
    int cost;
    const Operator *canonical_op;

    // HACK! because label_reducer uses const Label pointers, we must declare
    // add_mapping_label as const to be allowed to add mapping labels after
    // the creation of the (const) label
    mutable std::vector<const Label *> mapping_labels;
public:
    Label(int index, int cost, const Operator *op);
    Label(int index, const Label *label);
    void add_mapping_label(const Label *label) const;
    const Operator *get_canonical_op() const;
    int get_index() const {
        return index;
    }
    int get_cost() const {
        return cost;
    }
    void dump() const;

    mutable bool marker1, marker2; // HACK! HACK!
};

#endif // LABEL_H
