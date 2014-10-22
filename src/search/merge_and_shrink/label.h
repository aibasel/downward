#ifndef MERGE_AND_SHRINK_LABEL_H
#define MERGE_AND_SHRINK_LABEL_H

/*
  This class implements labels as used by merge-and-shrink transition systems.
  Labels are opaque tokens that have an associated cost.
*/

class Label {
    int cost;
public:
    explicit Label(int cost);
    ~Label() {}
    int get_cost() const {
        return cost;
    }
};

#endif
