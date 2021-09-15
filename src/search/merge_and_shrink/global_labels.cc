#include "global_labels.h"

#include "types.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
GlobalLabelsConstIterator::GlobalLabelsConstIterator(
    const vector<int> &label_costs, bool end)
    : label_costs(label_costs),
      current_index(end ? label_costs.size() : 0) {
    next_valid_index();
}

void GlobalLabelsConstIterator::next_valid_index() {
    while (current_index < label_costs.size()
           && label_costs[current_index] == -1) {
        ++current_index;
    }
}

void GlobalLabelsConstIterator::operator++() {
    ++current_index;
    next_valid_index();
}

GlobalLabels::GlobalLabels(vector<int> &&label_costs, int max_num_labels)
    : label_costs(move(label_costs)),
      max_num_labels(max_num_labels) {
}

void GlobalLabels::reduce_labels(const vector<int> &old_labels) {
    /*
      Even though we currently only support exact label reductions where
      reduced labels are of equal cost, to support non-exact label reductions,
      we compute the cost of the new label as the minimum cost of all old
      labels reduced to it to satisfy admissibility.
    */
    int new_label_cost = INF;
    for (size_t i = 0; i < old_labels.size(); ++i) {
        int old_label = old_labels[i];
        int cost = get_label_cost(old_label);
        if (cost < new_label_cost) {
            new_label_cost = cost;
        }
        label_costs[old_label] = -1;
    }
    label_costs.push_back(new_label_cost);
}

int GlobalLabels::get_num_active_labels() const {
    int result = 0;
    for (int label_cost : label_costs) {
        if (label_cost != -1) {
            ++result;
        }
    }
    return result;
}

int GlobalLabels::get_label_cost(int label) const {
    assert(label_costs[label] != -1);
    return label_costs[label];
}

void GlobalLabels::dump_labels() const {
    utils::g_log << "active labels:" << endl;
    for (size_t label = 0; label < label_costs.size(); ++label) {
        if (label_costs[label] != -1) {
            utils::g_log << "label " << label
                         << ", cost " << label_costs[label]
                         << endl;
        }
    }
}
}
