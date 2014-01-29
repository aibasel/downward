#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include "label.h"

#include "../globals.h"
#include "../operator.h"
#include "../operator_cost.h"

#include <cassert>
#include <vector>

class Abstraction;
class EquivalenceRelation;
class LabelSignature;

class LabelReducer {
    int num_reduced_labels;

    // approximative label reduction
    LabelSignature build_label_signature(const Label &label,
        const std::vector<bool> &var_is_used) const;
    int reduce_approximatively(const std::vector<int> &abs_vars,
                               std::vector<const Label *> &labels) const;

    // exact label reduction
    EquivalenceRelation *compute_outside_equivalence(const Abstraction *abstraction,
                                                     const std::vector<Abstraction *> &all_abstractions,
                                                     const std::vector<const Label *> &labels) const;
    int reduce_exactly(const EquivalenceRelation *relation, std::vector<const Label *> &labels) const;
public:
    LabelReducer(int abs_index,
                 const std::vector<Abstraction *> &all_abstractions,
                 std::vector<const Label* > &labels,
                 bool exact,
                 bool fixpoint);

    int get_number_reduced_labels() const {
        return num_reduced_labels;
    }
};

#endif
