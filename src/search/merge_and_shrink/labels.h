#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

// TODO: rename this file to global_labels.h

#include <memory>
#include <vector>

namespace merge_and_shrink {
class GlobalLabel {
    /*
      This class implements labels as used by merge-and-shrink transition
      systems. Labels are opaque tokens that have an associated cost.
    */
    int cost;
public:
    explicit GlobalLabel(int cost_)
        : cost(cost_) {
    }
    ~GlobalLabel() {}
    int get_cost() const {
        return cost;
    }
};

/*
  This class serves both as a container class to handle the set of all global
  labels and to perform label reduction on this set.
*/
class GlobalLabels {
    std::vector<std::unique_ptr<GlobalLabel>> labels;
    int max_size; // the maximum number of labels that can be created
public:
    explicit GlobalLabels(std::vector<std::unique_ptr<GlobalLabel>> &&labels);
    void reduce_labels(const std::vector<int> &old_label_nos);
    int get_num_active_labels() const;
    bool is_current_label(int label_no) const;
    int get_label_cost(int label_no) const;
    void dump_labels() const;
    int get_size() const {
        return labels.size();
    }
    int get_max_size() const {
        return max_size;
    }
};
}

#endif
