#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include <memory>
#include <vector>

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class Label {
    /*
      This class implements labels as used by merge-and-shrink transition systems.
      Labels are opaque tokens that have an associated cost.
    */
    int cost;
public:
    explicit Label(int cost_)
        : cost(cost_) {
    }
    ~Label() {}
    int get_cost() const {
        return cost;
    }
};

/*
  This class serves both as a container class to handle the set of all labels
  and to perform label reduction on this set.
*/
class Labels {
    std::vector<std::unique_ptr<Label>> labels;
    int max_size; // the maximum number of labels that can be created
public:
    explicit Labels(std::vector<std::unique_ptr<Label>> &&labels);
    void reduce_labels(const std::vector<int> &old_label_nos);
    bool is_current_label(int label_no) const;
    int get_label_cost(int label_no) const;
    void dump_labels(utils::LogProxy &log) const;
    int get_size() const {
        return labels.size();
    }
    int get_max_size() const {
        return max_size;
    }
};
}

#endif
