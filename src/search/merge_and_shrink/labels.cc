#include "labels.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
Labels::Labels(vector<unique_ptr<Label>> &&labels)
    : labels(move(labels)),
      max_size(0) {
    if (!this->labels.empty()) {
        max_size = this->labels.size() * 2 - 1;
    }
}

void Labels::reduce_labels(const vector<int> &old_label_nos) {
    int new_label_cost = labels[old_label_nos[0]]->get_cost();
    for (size_t i = 0; i < old_label_nos.size(); ++i) {
        int old_label_no = old_label_nos[i];
        labels[old_label_no] = nullptr;
    }
    labels.push_back(utils::make_unique_ptr<Label>(new_label_cost));
}

bool Labels::is_current_label(int label_no) const {
    assert(utils::in_bounds(label_no, labels));
    return labels[label_no] != nullptr;
}

int Labels::get_label_cost(int label_no) const {
    assert(labels[label_no]);
    return labels[label_no]->get_cost();
}

void Labels::dump_labels() const {
    cout << "active labels:" << endl;
    for (size_t label_no = 0; label_no < labels.size(); ++label_no) {
        if (labels[label_no]) {
            cout << "label " << label_no
                 << ", cost " << labels[label_no]->get_cost()
                 << endl;
        }
    }
}
}
