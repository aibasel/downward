#include "label.h"

Label::Label(int index_, int cost_, const Operator *op)
    : index(index_), cost(cost_), canonical_op(op) {
}

//void Label::add_mapped_operator(const Operator *op) {
//    mapped_operators.push_back(op);
//}

const Operator *Label::get_canonical_op() const {
    if (canonical_op) {
        return canonical_op;
    }
    assert(!mapped_labels.empty());
    return mapped_labels[0]->get_canonical_op();
}
