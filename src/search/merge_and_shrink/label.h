#ifndef LABEL_H
#define LABEL_H

#include "../operator.h"

#include <vector>

//class Operator;

class Label {
    int index;
    std::vector<const Operator *> mapped_operators;
public:
    Label(int index);
    void add_mapped_operator(const Operator *op);
};

#endif // LABEL_H
