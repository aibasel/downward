#include "label_equivalence_relation.h"

#include "labels.h"

#include <cassert>


using namespace std;

LabelEquivalenceRelation::LabelEquivalenceRelation(const std::shared_ptr<Labels> labels)
    : labels(labels) {
    label_to_positions.resize(labels->get_max_size());
}

void LabelEquivalenceRelation::add_label_to_group(LabelGroupIter group_it,
                                                  int label_no) {
    LabelIter label_it = group_it->insert(label_no);
    label_to_positions[label_no] = make_pair(group_it, label_it);

    int label_cost = labels->get_label_cost(label_no);
    if (label_cost < group_it->get_cost())
        group_it->set_cost(label_cost);
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

void LabelEquivalenceRelation::replace_labels_by_label(
    const vector<int> &old_label_nos, int new_label_no) {
    // Add new label to group
    LabelGroupIter group_it = label_to_positions[old_label_nos.front()].first;
    add_label_to_group(group_it, new_label_no);

    // Remove old labels from group
    for (int old_label_no : old_label_nos) {
        LabelIter label_it = label_to_positions[old_label_no].second;
        assert(group_it == label_to_positions[old_label_no].first);
        group_it->erase(label_it);
    }
}

LabelGroupIter LabelEquivalenceRelation::move_group_into_group(
    LabelGroupIter from_group, LabelGroupIter to_group) {
    for (LabelConstIter group2_label_it = from_group->begin();
         group2_label_it != from_group->end(); ++group2_label_it) {
        int other_label_no = *group2_label_it;
        add_label_to_group(to_group, other_label_no);
    }
    return grouped_labels.erase(from_group);
}

bool LabelEquivalenceRelation::erase(int label_no) {
    LabelGroupIter group_it = label_to_positions[label_no].first;
    group_it->erase(label_to_positions[label_no].second);
    if (group_it->empty()) {
        grouped_labels.erase(group_it);
        return true;
    }
    return false;
}

int LabelEquivalenceRelation::add_label_group(const vector<int> &new_labels) {
    int new_id = new_labels[0];
    LabelGroupIter group_it =
        grouped_labels.insert(grouped_labels.end(), LabelGroup(new_id));
    for (size_t i = 0; i < new_labels.size(); ++i) {
        int label_no = new_labels[i];
        add_label_to_group(group_it, label_no);
    }
    return new_id;
}

int LabelEquivalenceRelation::get_num_labels() const {
    return labels->get_size();
}
