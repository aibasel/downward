#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

#include "../operator.h"

#include <vector>

class CompositeLabel;

/* This class implements labels as used by merge-and-shrink abstractions.
   It is a wrapper class for regular operators that allows to store additional
   informations associated with labels.
   NOTE: operators that are axioms are currently not supported! */

class Label {
    friend class CompositeLabel;
    int id;
    int cost;
    // prevail and pre_posts are references to those of one "canonical"
    // operator, which is the operator an OperatorLabel was built from or
    // the "first" label of all parent labels when constructing a CompositeLabel.
    // TODO: can this be dealt with differently? E.g. only keep var/val pairs
    // from prevail/pre_post in composite labels where all parent labels agree
    // on?
    const std::vector<Prevail> &prevail;
    const std::vector<PrePost> &pre_post;
protected:
    // root is a pointer to a composite label that this label has been reduced
    // to, if such a label exists, or to itself, if the label has not been
    // reduced yet.
    // TODO: root is mutable because update_root must be const in order to be
    // applied to const Label pointers such as parents in CompositeLabel.
    // Parents are const on their turn because the Labels class keeps one
    // vector of const label pointers to all labels generated.
    mutable Label *root;

    Label(int id, int cost, const std::vector<Prevail> &prevail, const std::vector<PrePost> &pre_post);
    virtual ~Label() {}
    virtual void update_root(CompositeLabel *new_root) const = 0;
public:
    const std::vector<Prevail> &get_prevail() const {return prevail; }
    const std::vector<PrePost> &get_pre_post() const {return pre_post; }
    int get_id() const {
        return id;
    }
    int get_cost() const {
        return cost;
    }
    const Label *get_reduced_label() const {
        return root;
    }
    void dump() const;

    mutable bool marker1, marker2; // HACK! HACK!
};

class OperatorLabel : public Label {
    void update_root(CompositeLabel *new_root) const;
public:
    OperatorLabel(int id, int cost, const std::vector<Prevail> &prevail, const std::vector<PrePost> &pre_post);
};

class CompositeLabel : public Label {
    std::vector<const Label *> parents;
    void update_root(CompositeLabel *new_root) const;
public:
    CompositeLabel(int id, const std::vector<const Label *> &parents);
};

#endif
