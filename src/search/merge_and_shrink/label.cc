#include "label.h"

#include "../globals.h"

#include <ostream>

using namespace std;

Label::Label(int index_, int cost_, const Operator *op)
    : index(index_), cost(cost_), canonical_op(op) {
    marker1 = marker2 = false;
}

Label::Label(int index_, const Label *label)
    : index(index_), cost(label->get_cost()), canonical_op(0) {
    add_mapping_label(label);
}

void Label::add_mapping_label(const Label *label) const {
    assert(cost == label->get_cost());
    mapping_labels.push_back(label);
}

const Operator *Label::get_canonical_op() const {
    if (canonical_op) {
        return canonical_op;
    }
    assert(!mapping_labels.empty());
    return mapping_labels[0]->get_canonical_op();
}

void Label::dump() const {
    cout << "index: " << index << (index < g_operators.size() ? " regular operator" : "" ) << endl;
    cout << "cost: " << cost << endl;
    if (canonical_op) {
        cout << "canonical op:" << endl;
        canonical_op->dump();
        cout << "cost: " << canonical_op->get_cost() << endl;
    }
}
