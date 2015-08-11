#ifndef MERGE_AND_SHRINK_LABEL_GROUP_ITERATOR_H
#define MERGE_AND_SHRINK_LABEL_GROUP_ITERATOR_H

#include <list>
#include <memory>
#include <vector>

typedef std::list<int>::const_iterator LabelConstIter;

class LabelEquivalenceRelation;

class TSConstIterator {
    std::shared_ptr<LabelEquivalenceRelation> label_equivalence_relation;
    int current;
public:
    TSConstIterator(std::shared_ptr<LabelEquivalenceRelation> label_equivalence_relation,
                            bool end);
    // NOTE: not explicit because we copy and assign these iterators when
    // creating them.
    TSConstIterator(const TSConstIterator &other);
    void operator++();
    bool operator==(const TSConstIterator &rhs) const {
        return current == rhs.current;
    }
    bool operator!=(const TSConstIterator &rhs) const {
        return current != rhs.current;
    }
    int get_id() const {
        return current;
    }
    int get_cost() const;
    LabelConstIter begin() const;
    LabelConstIter end() const;
};


#endif
