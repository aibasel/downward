#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

#include "../operator.h"

#include <vector>

class CompositeLabel;

/* This class implements labels as used by merge-and-shrink abstractions.
   It abstracts from the underlying regular operators and allows to store
   additional information associated with labels.
   NOTE: operators that are axioms are currently not supported! */

class Label {
    friend class CompositeLabel; // required for update_root
    int id;
    int cost;
    // prevail and pre_posts are references to those of one "canonical"
    // operator, which is the operator an OperatorLabel was built from or
    // the "first" label of all parent labels when constructing a CompositeLabel.
    const std::vector<Prevail> &prevail;
    const std::vector<PrePost> &pre_post;
protected:
    // root is a pointer to a composite label that this label has been reduced
    // to, if such a label exists, or to itself, if the label has not been
    // reduced yet.
    Label *root;

    Label(int id, int cost, const std::vector<Prevail> &prevail,
          const std::vector<PrePost> &pre_post);
    virtual ~Label() {}
    virtual void update_root(CompositeLabel *new_root) = 0;
public:
    const std::vector<Prevail> &get_prevail() const {return prevail; }
    const std::vector<PrePost> &get_pre_post() const {return pre_post; }
    int get_id() const {
        return id;
    }
    int get_cost() const {
        return cost;
    }
    bool is_reduced() const;
    virtual const std::vector<Label *> &get_parents() const = 0;
    void dump() const;
};

class OperatorLabel : public Label {
    void update_root(CompositeLabel *new_root);
public:
    OperatorLabel(int id, int cost, const std::vector<Prevail> &prevail,
                  const std::vector<PrePost> &pre_post);

    const std::vector<Label *> &get_parents() const;
};

class CompositeLabel : public Label {
    std::vector<Label *> parents;

    void update_root(CompositeLabel *new_root);
public:
    CompositeLabel(int id, const std::vector<Label *> &parents);
    const std::vector<Label *> &get_parents() const {
        return parents;
    }
};

#endif
