#include "labels.h"

#include "label.h"
#include "label_reducer.h"

#include "../globals.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>

using namespace std;

Labels::Labels(const Options &options, OperatorCost cost_type) {
    label_reducer = new LabelReducer(options);
    if (!g_operators.empty()) {
        if (g_operators.size() * 2 - 1 > labels.max_size())
            exit_with(EXIT_OUT_OF_MEMORY);
        labels.reserve(g_operators.size() * 2 - 1);
    }
    for (size_t i = 0; i < g_operators.size(); ++i) {
        labels.push_back(new OperatorLabel(i, get_adjusted_action_cost(g_operators[i], cost_type),
                                           g_operators[i].get_preconditions(),
                                           g_operators[i].get_effects()));
    }
}

Labels::~Labels() {
    delete label_reducer;
}

void Labels::reduce(pair<int, int> next_merge,
                    const std::vector<TransitionSystem *> &all_transition_systems) {
    label_reducer->reduce_labels(next_merge, all_transition_systems, labels);
}

const Label *Labels::get_label_by_index(int index) const {
    assert(in_bounds(index, labels));
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
