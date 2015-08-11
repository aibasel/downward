#include "label_equivalence_relation.h"

#include "labels.h"

#include <cassert>


using namespace std;

LabelEquivalenceRelation::LabelEquivalenceRelation(const std::shared_ptr<Labels> labels)
    : labels(labels) {
    grouped_labels.reserve(labels->get_max_size());
    label_to_positions.resize(labels->get_max_size());
}

void LabelEquivalenceRelation::add_label_to_group(int group_id,
                                                  int label_no) {
    LabelIter label_it = grouped_labels[group_id].insert(label_no);
    label_to_positions[label_no] = make_pair(group_id, label_it);

    int label_cost = labels->get_label_cost(label_no);
    if (label_cost < grouped_labels[group_id].get_cost())
        grouped_labels[group_id].set_cost(label_cost);
}

void LabelEquivalenceRelation::recompute_group_cost() {
    for (LabelGroup &label_group : grouped_labels) {
        if (!label_group.empty()) {
            // TODO: duplication of INF in transition_system.h
            label_group.set_cost(numeric_limits<int>::max());
            for (LabelConstIter label_it = label_group.begin();
                 label_it != label_group.end(); ++label_it) {
                int cost = labels->get_label_cost(*label_it);
                if (cost < label_group.get_cost()) {
                    label_group.set_cost(cost);
                }
            }
        }
    }
}

void LabelEquivalenceRelation::replace_labels_by_label(
    const vector<int> &old_label_nos, int new_label_no) {
    // Add new label to group
    int group_id = get_group_id(old_label_nos.front());
    add_label_to_group(group_id, new_label_no);

    // Remove old labels from group
    for (int old_label_no : old_label_nos) {
        LabelIter label_it = label_to_positions[old_label_no].second;
        assert(group_id == get_group_id(old_label_no));
        grouped_labels[group_id].erase(label_it);
    }
}

void LabelEquivalenceRelation::move_group_into_group(
    int from_group_id, int to_group_id) {
    LabelGroup &from_group = grouped_labels[from_group_id];
    for (LabelConstIter from_label_it = from_group.begin();
         from_label_it != from_group.end(); ++from_label_it) {
        int from_label_no = *from_label_it;
        add_label_to_group(to_group_id, from_label_no);
    }
    from_group.clear();
}

bool LabelEquivalenceRelation::erase(int label_no) {
    int group_id = get_group_id(label_no);
    LabelIter label_it = label_to_positions[label_no].second;
    grouped_labels[group_id].erase(label_it);
    return grouped_labels[group_id].empty();
}

int LabelEquivalenceRelation::add_label_group(const vector<int> &new_labels) {
    int new_id = grouped_labels.size();
    grouped_labels.push_back(LabelGroup());
    for (size_t i = 0; i < new_labels.size(); ++i) {
        int label_no = new_labels[i];
        add_label_to_group(new_id, label_no);
    }
    return new_id;
}

int LabelEquivalenceRelation::get_num_labels() const {
    return labels->get_size();
}
