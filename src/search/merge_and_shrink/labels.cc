#include "labels.h"

#include "label.h"
#include "label_reducer.h"
#include "../globals.h"

#include <cassert>

using namespace std;

Labels::Labels(OperatorCost cost_type) {
    //reduced_label_by_index.resize(g_operators.size() * 2, 0);
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new OperatorLabel(i, get_adjusted_action_cost(g_operators[i], cost_type),
                                   g_operators[i].get_prevail(), g_operators[i].get_pre_post()));
        //reduced_label_by_index[i] = labels[i];
    }
}

Labels::~Labels() {
}

void Labels::reduce_labels(const std::vector<const Label *> &relevant_labels,
                           const std::vector<int> &pruned_vars) {
    LabelReducer label_reducer(relevant_labels, pruned_vars, labels);
    label_reducer.statistics();
    /*const vector<const Label *> &temp = label_reducer.get_reduced_label_by_id();
    for (size_t i = 0; i < labels.size(); ++i) {
        if (i < temp.size()) {
            if (temp[i] != 0) {
                reduced_label_by_index[i] = temp[i];
                assert(get_label_by_index(i)->get_reduced_label() == temp[i]);
            }
        } else {
            reduced_label_by_index[i] = labels[i];
        }
    }*/
}

int Labels::get_reduced_label_no(int label_no) const {
    /*int current_label_no = label_no;
    while (reduced_label_by_index[current_label_no]->get_id() != current_label_no) {
        current_label_no = reduced_label_by_index[current_label_no]->get_id();
    }
    assert(get_label_by_index(label_no)->get_reduced_label() == get_label_by_index(current_label_no));
    assert(get_label_by_index(label_no)->get_reduced_label()->get_id() == current_label_no);
    return current_label_no;*/
    return get_label_by_index(label_no)->get_reduced_label()->get_id();
}

/*const Label *Labels::get_reduced_label(const Label *label) const {
    int label_no = label->get_id();
    int reduced_label_no = get_reduced_label_no(label_no);
    const Label *reduced_label = labels[reduced_label_no];
    return reduced_label;
}*/

const Label *Labels::get_label_by_index(int index) const {
    assert(index >= 0 && index < labels.size());
    return labels[index];
}

void Labels::dump() const {
    cout << "no of labels: " << labels.size() << endl;
    /*for (size_t i = 0; i < labels.size(); ++i) {
        cout << "label " << i << endl;
        labels[i]->dump();
        cout << "mapped to: " << endl;
        reduced_label_by_index[i]->dump();
        cout << endl;
    }*/
}
