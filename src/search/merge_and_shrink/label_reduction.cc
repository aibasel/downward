#include "label_reduction.h"

#include "label.h"
#include "label_reducer.h"
#include "../globals.h"

#include <cassert>

LabelReduction::LabelReduction(const Options &options_)
    : options(options_), label_reducer(0), next_label_no(g_operators.size()) {
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new Label(i, get_adjusted_action_cost(g_operators[i], OperatorCost(options.get_enum("cost_type"))), &g_operators[i]));
    }
}

void LabelReduction::reduce_labels(const std::vector<const Label *> &relevant_labels,
              const std::vector<int> &pruned_vars) {
    label_reducer = new LabelReducer(relevant_labels, pruned_vars);
    label_reducer->statistics();
}

int LabelReduction::get_reduced_label(int label_no) const {
    return label_reducer->get_reduced_label(label_no);
}

void LabelReduction::free() {
    if (label_reducer) {
        delete label_reducer;
    }
}

int LabelReduction::get_cost_for_label(int label_no) const {
    assert(label_no >= 0 && label_no < labels.size());
    return labels[label_no]->get_cost();
}

const Label *LabelReduction::get_label_by_index(int index) const {
    assert(index >= 0 && index < labels.size());
    return labels[index];
}
