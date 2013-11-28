#include "labels.h"

#include "label.h"
#include "label_reducer.h"
#include "../globals.h"

#include <cassert>

Labels::Labels(OperatorCost cost_type)
    : label_reducer(0) {
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new Label(i, get_adjusted_action_cost(g_operators[i], cost_type), &g_operators[i]));
    }
}

Labels::~Labels() {
    free();
}

void Labels::reduce_labels(const std::vector<const Label *> &relevant_labels,
                           const std::vector<int> &pruned_vars) {
    assert(!label_reducer);
    label_reducer = new LabelReducer(relevant_labels, pruned_vars, labels);
    label_reducer->statistics();
}

int Labels::get_reduced_label(int label_no) const {
    assert(label_reducer);
    return label_reducer->get_reduced_label(label_no);
}

void Labels::free() {
    if (label_reducer) {
        delete label_reducer;
        label_reducer = 0;
    }
}

int Labels::get_cost_for_label(int label_no) const {
    assert(label_no >= 0 && label_no < labels.size());
    return labels[label_no]->get_cost();
}

const Label *Labels::get_label_by_index(int index) const {
    assert(index >= 0 && index < labels.size());
    return labels[index];
}

void Labels::dump() const {
    for (size_t i = 0; i < labels.size(); ++i) {
        labels[i]->dump();
    }
}
