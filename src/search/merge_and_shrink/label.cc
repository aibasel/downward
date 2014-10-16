#include "label.h"

#include <iostream>

using namespace std;

Label::Label(int id_, int cost_)
    : id(id_), cost(cost_) {
}

OperatorLabel::OperatorLabel(int id, int cost,
                             const vector<GlobalCondition> &preconditions_,
                             const vector<GlobalEffect> &effects_)
    : Label(id, cost), preconditions(preconditions_), effects(effects_) {
}

CompositeLabel::CompositeLabel(int id, int cost)
    : Label(id, cost) {
}

void Label::dump() const {
    cout << "label: " << id << ", cost: " << cost << endl;
}
