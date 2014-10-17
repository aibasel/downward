#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

/* This class implements labels as used by merge-and-shrink transition systems.
   It abstracts from the underlying regular operators and allows to store
   additional information associated with labels.
   NOTE: operators that are axioms are currently not supported! */

class Label {
    int id;
    int cost;
public:
    Label(int id, int cost);
    ~Label() {}
    int get_id() const {
        return id;
    }
    int get_cost() const {
        return cost;
    }
    void dump() const;
};

#endif
