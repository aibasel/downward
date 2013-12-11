#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

#include "../operator.h"

#include <vector>

/* This class implements labels as used by merge-and-shrink abstractions.
   It is a wrapper class for regular operators that allows to store additional
   informations associated with labels.
   NOTE: operators that are axioms are currently not supported! */

class Label {
protected:
    int id;
    int cost;
    const std::vector<Prevail> prevail;
    const std::vector<PrePost> pre_post;
    const Label *root;
public:
    Label(int id, int cost, const std::vector<Prevail> &prevail, const std::vector<PrePost> &pre_post);
    void add_mapping_label(const Label *label) const;
    const std::vector<Prevail> &get_prevail() const {return prevail; }
    const std::vector<PrePost> &get_pre_post() const {return pre_post; }
    int get_id() const {
        return id;
    }
    int get_cost() const {
        return cost;
    }
    void dump() const;

    mutable bool marker1, marker2; // HACK! HACK!
};

class SingleLabel : public Label {
public:
    SingleLabel(int id, int cost, const std::vector<Prevail> &prevail, const std::vector<PrePost> &pre_post);
};

class CompositeLabel : public Label {
    const std::vector<const Label *> parents;
public:
    CompositeLabel(int id, const std::vector<const Label *> &labels);
};

#endif
