#include "labels.h"

#include "label.h"
#include "label_reducer.h"

#include "../globals.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>

using namespace std;

Labels::Labels(OperatorCost cost_type, LabelReduction label_reduction_,
               FixpointVariableOrder fix_point_variable_order_)
    : label_reduction(label_reduction_), fix_point_variable_order(fix_point_variable_order_) {
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
        exit_with(EXIT_UNSUPPORTED);
        break;
    case APPROXIMATIVE_WITH_FIXPOINT:
        exit_with(EXIT_UNSUPPORTED);
        fixpoint = true;
        break;
    case EXACT:
        exact = true;
        break;
    case EXACT_WITH_FIXPOINT:
        exact = true;
        fixpoint = true;
        for (int i = 0; i < g_variable_domain.size(); ++i) {
            if (fix_point_variable_order == NATURAL
                    || fix_point_variable_order == RANDOM) {
                variable_order.push_back(i);
            } else if (fix_point_variable_order == REVERSE) {
                variable_order.push_back(g_variable_domain.size() - 1 - i);
            } else {
                exit_with(EXIT_INPUT_ERROR);
            }
        }
        if (fix_point_variable_order == RANDOM) {
            random_shuffle(variable_order.begin(), variable_order.end());
        }
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
    LabelReducer label_reducer(abs_index, all_abstractions, labels,
                               exact, fixpoint, variable_order);
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
    if (label_reduction == EXACT_WITH_FIXPOINT) {
        cout << "Fixpoint variable order: ";
        switch (fix_point_variable_order) {
        case NATURAL:
            cout << "natural";
            break;
        case REVERSE:
            cout << "reversed";
            break;
        case RANDOM:
            cout << "random";
            break;
        }
        cout << endl;
        cout << variable_order << endl;
    }
}
