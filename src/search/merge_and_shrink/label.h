#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

#include <vector>

class GlobalCondition;
class GlobalEffect;

/* This class implements labels as used by merge-and-shrink transition systems.
   It abstracts from the underlying regular operators and allows to store
   additional information associated with labels.
   NOTE: operators that are axioms are currently not supported! */

class Label {
    int id;
    int cost;
protected:
    Label(int id, int cost);
public:
    virtual ~Label() {}
    int get_id() const {
        return id;
    }
    int get_cost() const {
        return cost;
    }
    void dump() const;
};

class OperatorLabel : public Label {
    const std::vector<GlobalCondition> &preconditions;
    const std::vector<GlobalEffect> &effects;
public:
    OperatorLabel(int id, int cost,
                  const std::vector<GlobalCondition> &preconditions,
                  const std::vector<GlobalEffect> &effects);
    const std::vector<GlobalCondition> &get_preconditions() const {
        return preconditions;
    }
    const std::vector<GlobalEffect> &get_effects() const {
        return effects;
    }
};

class CompositeLabel : public Label {
public:
    CompositeLabel(int id, int cost);
};

#endif
