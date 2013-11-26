#include "label.h"

Label::Label(int index_)
    : index(index_) {
}

void Label::add_mapped_operator(const Operator *op) {
    mapped_operators.push_back(op);
}
