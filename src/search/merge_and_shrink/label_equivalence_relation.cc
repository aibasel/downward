#include "label_equivalence_relation.h"

#include "labels.h"


using namespace std;

LabelEquivalenceRelation::LabelEquivalenceRelation(const std::shared_ptr<Labels> labels)
    : labels(labels) {
    label_to_positions.resize(labels->get_max_size());
}

void LabelEquivalenceRelation::recompute_group_cost() {
    for (LabelGroupIter group_it = grouped_labels.begin();
         group_it != grouped_labels.end(); ++group_it) {
        // TODO: duplication of INF in transition_system.h
        group_it->set_cost(numeric_limits<int>::max());
        for (LabelConstIter label_it = group_it->begin();
             label_it != group_it->end(); ++label_it) {
            int cost = labels->get_label_cost(*label_it);
            if (cost < group_it->get_cost()) {
                group_it->set_cost(cost);
            }
        }
    }
}

void LabelEquivalenceRelation::add_label_to_group(LabelGroupIter group_it,
                                                  int label_no) {
    LabelIter label_it = group_it->insert(label_no);
    label_to_positions[label_no] = make_pair(group_it, label_it);

    int label_cost = labels->get_label_cost(label_no);
    if (label_cost < group_it->get_cost())
        group_it->set_cost(label_cost);
}

int LabelEquivalenceRelation::add_label_group(const vector<int> &new_labels) {
    int new_index = new_labels[0];
    LabelGroupIter group_it = add_empty_label_group(new_index);
    for (size_t i = 0; i < new_labels.size(); ++i) {
        int label_no = new_labels[i];
        add_label_to_group(group_it, label_no);
    }
    return new_index;
}

int LabelEquivalenceRelation::get_num_labels() const {
    return labels->get_size();
}