#include "labels.h"

#include "label.h"
#include "label_reducer.h"
#include "../globals.h"

#include <cassert>

using namespace std;

Labels::Labels(OperatorCost cost_type) {
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new OperatorLabel(i, get_adjusted_action_cost(g_operators[i], cost_type),
                                           g_operators[i].get_prevail(), g_operators[i].get_pre_post()));
    }
}

Labels::~Labels() {
}

int Labels::reduce_approximatively(const std::vector<const Label *> &relevant_labels,
                                    const std::vector<int> &abs_vars) {
    LabelReducer label_reducer(relevant_labels, abs_vars, labels);
    label_reducer.statistics();
    return label_reducer.get_no_reduced_labels();
}

int Labels::reduce_exactly(const vector<const Label *> &relevant_labels,
                            const EquivalenceRelation *relation) {
    LabelReducer label_reducer(relevant_labels, relation, labels);
    label_reducer.statistics2();
    return label_reducer.get_no_reduced_labels();
}


int Labels::get_reduced_label_no(int label_no) const {
    return get_label_by_index(label_no)->get_reduced_label()->get_id();
}

const Label *Labels::get_label_by_index(int index) const {
    assert(index >= 0 && index < labels.size());
    return labels[index];
}

bool Labels::is_label_reduced(int label_no) const {
    return label_no != get_reduced_label_no(label_no);
}

void Labels::dump() const {
    cout << "no of labels: " << labels.size() << endl;
    for (size_t i = 0; i < labels.size(); ++i) {
        const Label *label = labels[i];
        cout << label->get_id() << "->" << label->get_reduced_label()->get_id() << endl;
    }
}
