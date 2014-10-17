#include "label.h"

#include <iostream>

using namespace std;

Label::Label(int id_, int cost_)
    : id(id_), cost(cost_) {
}

void Label::dump() const {
    cout << "label: " << id << ", cost: " << cost << endl;
}
