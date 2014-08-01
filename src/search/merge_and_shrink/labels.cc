#include "labels.h"

#include "label.h"
#include "label_reducer.h"

#include "../globals.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>

using namespace std;

Labels::Labels(bool unit_cost_, const Options &options, OperatorCost cost_type)
    : unit_cost(unit_cost_) {
    label_reducer = new LabelReducer(options);
    if (!g_operators.empty())
        labels.reserve(g_operators.size() * 2 - 1);
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new OperatorLabel(i, get_adjusted_action_cost(g_operators[i], cost_type),
                                           g_operators[i].get_prevail(), g_operators[i].get_pre_post()));
    }
}

Labels::~Labels() {
    delete label_reducer;
}

void Labels::reduce(pair<int, int> next_merge,
                    const std::vector<Abstraction *> &all_abstractions) {
    label_reducer->reduce_labels(next_merge, all_abstractions, labels);
}

const Label *Labels::get_label_by_index(int index) const {
    assert(index >= 0 && index < labels.size());
    return labels[index];
}

bool Labels::is_label_reduced(int label_no) const {
    return get_label_by_index(label_no)->is_reduced();
}

void Labels::dump() const {
    cout << "no of labels: " << labels.size() << endl;
    for (size_t i = 0; i < labels.size(); ++i) {
        const Label *label = labels[i];
        label->dump();
    }
}

void Labels::dump_options() const {
    label_reducer->dump_options();
}
