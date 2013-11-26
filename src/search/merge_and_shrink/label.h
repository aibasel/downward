#ifndef LABEL_H
#define LABEL_H

#include "../operator.h"

#include <vector>

//class Operator;

class Label {
    int index;
    int cost;
    const Operator *canonical_op;

    std::vector<const Label *> mapped_labels;
public:
    Label(int index, int cost, const Operator *op);
    //void add_mapped_operator(const Operator *op);
    const Operator *get_canonical_op() const;
    int get_index() const {
        return index;
    }
    int get_cost() const {
        return cost;
    }

    mutable bool marker1, marker2; // HACK! HACK!
};

#endif // LABEL_H
