#ifndef MERGE_AND_SHRINK_LABEL_EQUIVALENCE_RELATION_H
#define MERGE_AND_SHRINK_LABEL_EQUIVALENCE_RELATION_H

#include <limits>
#include <list>
#include <memory>
#include <vector>


class Labels;

typedef std::list<int>::iterator LabelIter;
typedef std::list<int>::const_iterator LabelConstIter;

class LabelGroup {
    /*
      A label group contains a set of locally equivalent labels, possibly of
      different cost, and stores the minimum cost of all labels of the group.
    */
    std::list<int> labels;
    int cost;
public:
    LabelGroup()
    // TODO: duplication of INF in transition_system.h
        : cost(std::numeric_limits<int>::max()) {
    }
    void set_cost(int cost_) {
        cost = cost_;
    }
    LabelIter insert(int label) {
        return labels.insert(labels.end(), label);
    }
    void erase(LabelIter pos) {
        labels.erase(pos);
    }
    void clear() {
        labels.clear();
    }
    LabelIter begin() {
        return labels.begin();
    }
    LabelConstIter begin() const {
        return labels.begin();
    }
    LabelIter end() {
        return labels.end();
    }
    LabelConstIter end() const {
        return labels.end();
    }
    bool empty() const {
        return labels.empty();
    }
    int size() const {
        return labels.size();
    }
    int get_cost() const {
        return cost;
    }
};

class LabelEquivalenceRelation {
    /*
      There should only be one instance of Labels at runtime. It is created
      and managed by MergeAndShrinkHeuristic.
    */
    const std::shared_ptr<Labels> labels;

    std::vector<LabelGroup> grouped_labels;
    // maps each label to its group's id and its iterator within the group.
    std::vector<std::pair<int, LabelIter> > label_to_positions;

    void add_label_to_group(int group_id, int label_no);
public:
    explicit LabelEquivalenceRelation(const std::shared_ptr<Labels> labels);
    virtual ~LabelEquivalenceRelation() = default;

    void recompute_group_cost();
    void replace_labels_by_label(
        const std::vector<int> &old_label_nos, int new_label_no);
    void move_group_into_group(int from_group_id, int to_group_id);
    bool erase(int label_no);
    int add_label_group(const std::vector<int> &new_labels);
    int get_group_id(int label_no) const {
        return label_to_positions[label_no].first;
    }
    int get_size() const {
        return grouped_labels.size();
    }
    int get_num_labels() const;
    const LabelGroup &operator[](int group_id) {
        return grouped_labels[group_id];
    }
};

#endif
