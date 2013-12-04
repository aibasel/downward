#include "labels.h"

#include "label.h"
#include "label_reducer.h"
#include "../globals.h"

#include <cassert>

using namespace std;

Labels::Labels(OperatorCost cost_type)
    : label_reducer(0) {
    reduced_label_by_index.resize(g_operators.size() * 2, 0);
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new Label(i, get_adjusted_action_cost(g_operators[i], cost_type), &g_operators[i]));
        reduced_label_by_index[i] = labels[i];
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
    const vector<const Label *> &temp = label_reducer->get_reduced_label_by_index();
    for (size_t i = 0; i < labels.size(); ++i) {
        if (i < temp.size()) {
            if (temp[i] != 0)
                reduced_label_by_index[i] = temp[i];
        } else {
            reduced_label_by_index[i] = labels[i];
        }
    }
}

int Labels::get_reduced_label(int label_no) const {
    //assert(label_reducer);
    //return label_reducer->get_reduced_label(label_no);
    int current_label_no = label_no;
    while (reduced_label_by_index[current_label_no]->get_index() != current_label_no) {
        current_label_no = reduced_label_by_index[current_label_no]->get_index();
    }
    return current_label_no;
}

const Label *Labels::get_red_label(const Label *label) const {
    int label_no = label->get_index();
    int reduced_label_no = get_reduced_label(label_no);
    const Label *reduced_label = get_label_by_index(reduced_label_no);
    // TODO: have a function return label by label_index that can be used by get_reduced_label and get_red_label.
    // Or only use numbers in abstraction and access labels only via get_label_by_index
    return reduced_label;
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
    cout << "no of labels: " << labels.size() << endl;
    for (size_t i = 0; i < labels.size(); ++i) {
        cout << "label " << i << endl;
        labels[i]->dump();
        cout << "mapped to: " << endl;
        reduced_label_by_index[i]->dump();
        cout << endl;
    }
}
