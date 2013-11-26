#ifndef LABEL_REDUCTION_H
#define LABEL_REDUCTION_H

#include "../option_parser.h"

#include <vector>

class LabelReducer;
class Operator;

class LabelReduction {
    const Options options;
    LabelReducer *label_reducer;
    int next_label_no;
public:
    LabelReduction(const Options &options);
    void reduce_labels(const std::vector<const Operator *> &relevant_operators,
                  const std::vector<int> &pruned_vars,
                  OperatorCost cost_type);

    const Operator *get_reduced_label(const Operator *op) const;
    void free();
};

#endif // LABEL_REDUCTION_H
