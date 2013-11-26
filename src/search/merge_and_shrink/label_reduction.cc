#include "label_reduction.h"

#include "label_reducer.h"
#include "../globals.h"

LabelReduction::LabelReduction(const Options &options_)
    : options(options_), label_reducer(0), next_label_no(g_operators.size()) {
}

void LabelReduction::reduce_labels(const std::vector<const Operator *> &relevant_operators,
              const std::vector<int> &pruned_vars,
              OperatorCost cost_type) {
    label_reducer = new LabelReducer(relevant_operators, pruned_vars, cost_type);
    label_reducer->statistics();
}

const Operator *LabelReduction::get_reduced_label(const Operator *op) const {
    return label_reducer->get_reduced_label(op);
}

void LabelReduction::free() {
    if (label_reducer) {
        delete label_reducer;
    }
}
