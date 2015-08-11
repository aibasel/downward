#include "ts_iterator.h"

#include "label_equivalence_relation.h"

using namespace std;

TSConstIterator::TSConstIterator(
        shared_ptr<LabelEquivalenceRelation> label_equivalence_relation,
        bool end)
    : label_equivalence_relation(label_equivalence_relation),
      current((end ? label_equivalence_relation->get_size() : 0)) {
    while (current < label_equivalence_relation->get_size()
            && (*label_equivalence_relation)[current].empty()) {
        ++current;
    }
}

TSConstIterator::TSConstIterator(const TSConstIterator &other)
    : label_equivalence_relation(other.label_equivalence_relation),
      current(other.current) {
    if (current < label_equivalence_relation->get_size()) {
        ++current;
    }
    while (current < label_equivalence_relation->get_size()
           && (*label_equivalence_relation)[current].empty()) {
        ++current;
    }
}

void TSConstIterator::operator++() {
    ++current;
    while (current < label_equivalence_relation->get_size()
           && (*label_equivalence_relation)[current].empty()) {
        ++current;
    }
}

int TSConstIterator::get_cost() const {
    return (*label_equivalence_relation)[current].get_cost();
}

LabelConstIter TSConstIterator::begin() const {
    return (*label_equivalence_relation)[current].begin();
}

LabelConstIter TSConstIterator::end() const {
    return (*label_equivalence_relation)[current].end();
}
