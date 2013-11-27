#include "label.h"

#include <ostream>

using namespace std;

Label::Label(int index_, int cost_, const Operator *op)
    : index(index_), cost(cost_), canonical_op(op) {
    marker1 = marker2 = false;
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

void Label::dump() const {
    cout << "index: " << index << endl;
    cout << "cost: " << cost << endl;
    if (canonical_op) {
        cout << "canonical op:" << endl;
        canonical_op->dump();
        cout << "cost: " << canonical_op->get_cost() << endl;
    }
}
