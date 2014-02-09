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
    friend class CompositeLabel;
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
    const Label *get_reduced_label() const {
        return root;
    }
    bool is_reduced() const;
    virtual void get_origins(std::vector<const Label *> &origins) const = 0;
    virtual const std::vector<Label *> &get_parents() const = 0;
    void dump() const;

    mutable bool marker1, marker2; // HACK! HACK!
};

class OperatorLabel : public Label {
    void update_root(CompositeLabel *new_root);

    const Operator *op; // for debugging only
public:
    OperatorLabel(const Operator *op,
                  int id, int cost, const std::vector<Prevail> &prevail,
                  const std::vector<PrePost> &pre_post);

    void get_origins(std::vector<const Label *> &origins) const;
    const std::vector<Label *> &get_parents() const;

    const Operator *get_operator() const {return op; }
};

class CompositeLabel : public Label {
    std::vector<Label *> parents;

    void update_root(CompositeLabel *new_root);
public:
    CompositeLabel(int id, const std::vector<Label *> &parents);
    void get_origins(std::vector<const Label *> &origins) const;
    const std::vector<Label *> &get_parents() const {
        return parents;
    }
};

#endif
