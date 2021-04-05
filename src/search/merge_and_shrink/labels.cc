#include "labels.h"

// TODO: rename this file to global_labels.cc

#include "types.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
GlobalLabels::GlobalLabels(vector<unique_ptr<GlobalLabel>> &&labels, int max_num_labels)
    : labels(move(labels)),
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
        labels[old_label] = nullptr;
    }
    labels.push_back(utils::make_unique_ptr<GlobalLabel>(new_label_cost));
}

int GlobalLabels::get_num_active_labels() const {
    int result = 0;
    for (const auto &label : labels) {
        if (label) {
            ++result;
        }
    }
    return result;
}

bool GlobalLabels::is_current_label(int label) const {
    assert(utils::in_bounds(label, labels));
    return labels[label] != nullptr;
}

int GlobalLabels::get_label_cost(int label) const {
    assert(labels[label]);
    return labels[label]->get_cost();
}

void GlobalLabels::dump_labels() const {
    utils::g_log << "active labels:" << endl;
    for (size_t label = 0; label < labels.size(); ++label) {
        if (labels[label]) {
            utils::g_log << "label " << label
                         << ", cost " << labels[label]->get_cost()
                         << endl;
        }
    }
}
}
