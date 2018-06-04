#include "label_equivalence_relation.h"

#include "labels.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
LabelEquivalenceRelation::LabelEquivalenceRelation(const Labels &labels)
    : labels(labels) {
    grouped_labels.reserve(labels.get_max_size());
    label_to_positions.resize(labels.get_max_size());
}

LabelEquivalenceRelation::LabelEquivalenceRelation(
    const LabelEquivalenceRelation &other)
    : labels(other.labels),
      /* We copy label_to_positions to have identical vectors even on
      "unused" positions (for label numbers that do not exist any more). */
      label_to_positions(other.label_to_positions) {
    /*
      We need to reserve space for the potential maximum number of labels to
      ensure that no move occurs in grouped_labels. Otherwise, iterators to
      elements of list<int> of LabelGroup could become invalid!
    */
    grouped_labels.reserve(labels.get_max_size());
    for (size_t other_group_id = 0;
         other_group_id < other.grouped_labels.size();
         ++other_group_id) {
        // Add a new empty label group.
        int group_id = grouped_labels.size();
        assert(group_id == static_cast<int>(other_group_id));
        grouped_labels.push_back(LabelGroup());
        LabelGroup &label_group = grouped_labels.back();

        /*
          Go over the other label group, add all labels to this group.

          To obtain exact copies of the label groups with the same cost, we do
          not use add_label_to_group, which would recompute costs based on
          given labels and leave cost=infinity for empty groups, but we
          manually set the group's cost to match the other group's cost.
        */
        const LabelGroup &other_label_group =
            other.grouped_labels[other_group_id];
        for (int other_label_no : other_label_group) {
            LabelIter label_it = label_group.insert(other_label_no);
            assert(*label_it == other_label_no);
            label_to_positions[other_label_no] = make_pair(group_id, label_it);
        }
        label_group.set_cost(other_label_group.get_cost());
    }
}

void LabelEquivalenceRelation::add_label_to_group(int group_id,
                                                  int label_no) {
    LabelIter label_it = grouped_labels[group_id].insert(label_no);
    label_to_positions[label_no] = make_pair(group_id, label_it);

    int label_cost = labels.get_label_cost(label_no);
    if (label_cost < grouped_labels[group_id].get_cost())
        grouped_labels[group_id].set_cost(label_cost);
}

void LabelEquivalenceRelation::apply_label_mapping(
    const vector<pair<int, vector<int>>> &label_mapping,
    const unordered_set<int> *affected_group_ids) {
    for (const pair<int, vector<int>> &mapping : label_mapping) {
        int new_label_no = mapping.first;
        const vector<int> &old_label_nos = mapping.second;

        // Add new label to group
        int canonical_group_id = get_group_id(old_label_nos.front());
        if (!affected_group_ids) {
            add_label_to_group(canonical_group_id, new_label_no);
        } else {
            add_label_group({new_label_no});
        }

        // Remove old labels from group
        for (int old_label_no : old_label_nos) {
            if (!affected_group_ids) {
                assert(canonical_group_id == get_group_id(old_label_no));
            }
            LabelIter label_it = label_to_positions[old_label_no].second;
            grouped_labels[get_group_id(old_label_no)].erase(label_it);
        }
    }

    if (affected_group_ids) {
        // Recompute the cost of all affected label groups.
        const unordered_set<int> &group_ids = *affected_group_ids;
        for (int group_id : group_ids) {
            LabelGroup &label_group = grouped_labels[group_id];
            // Setting cost to infinity for empty groups does not hurt.
            label_group.set_cost(INF);
            for (int label_no : label_group) {
                int cost = labels.get_label_cost(label_no);
                if (cost < label_group.get_cost()) {
                    label_group.set_cost(cost);
                }
            }
        }
    }
}

void LabelEquivalenceRelation::move_group_into_group(
    int from_group_id, int to_group_id) {
    assert(!is_empty_group(from_group_id));
    assert(!is_empty_group(to_group_id));
    LabelGroup &from_group = grouped_labels[from_group_id];
    for (int label_no : from_group) {
        add_label_to_group(to_group_id, label_no);
    }
    from_group.clear();
}

int LabelEquivalenceRelation::add_label_group(const vector<int> &new_labels) {
    int new_group_id = grouped_labels.size();
    grouped_labels.push_back(LabelGroup());
    for (int label_no : new_labels) {
        add_label_to_group(new_group_id, label_no);
    }
    return new_group_id;
}
}
