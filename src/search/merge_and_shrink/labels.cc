#include "labels.h"

#include "label.h"
#include "label_reducer.h"

#include "../globals.h"
#include "../utilities.h"

#include <cassert>

using namespace std;

Labels::Labels(OperatorCost cost_type, LabelReduction label_reduction_)
    : label_reduction(label_reduction_) {
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new OperatorLabel(i, get_adjusted_action_cost(g_operators[i], cost_type),
                                           g_operators[i].get_prevail(), g_operators[i].get_pre_post()));
    }
    exact = false;
    fixpoint = false;
    switch (label_reduction) {
    case NONE:
        break;
    case APPROXIMATIVE:
        break;
    case APPROXIMATIVE_WITH_FIXPOINT:
        fixpoint = true;
        break;
    case EXACT:
        exact = true;
        break;
    case EXACT_WITH_FIXPOINT:
        exact = true;
        fixpoint = true;
        break;
    default:
        exit_with(EXIT_INPUT_ERROR);
    }
}

Labels::~Labels() {
}

int Labels::reduce(int abs_index,
                   const std::vector<Abstraction *> &all_abstractions) {
    if (label_reduction == NONE) {
        return 0;
    }
    LabelReducer label_reducer(abs_index, all_abstractions, labels, exact, fixpoint);
    return label_reducer.get_number_reduced_labels();
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

void Labels::dump_options() const {
    cout << "Label reduction: ";
    switch (label_reduction) {
    case NONE:
        cout << "disabled";
        break;
    case APPROXIMATIVE:
        cout << "approximative";
        break;
    case APPROXIMATIVE_WITH_FIXPOINT:
        cout << "approximative with fixpoint computation";
        break;
    case EXACT:
        cout << "exact";
        break;
    case EXACT_WITH_FIXPOINT:
        cout << "exact with fixpoint computation";
        break;
    }
    cout << endl;
}
