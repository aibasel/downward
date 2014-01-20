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

// TODO: possible refactoring: ApproximateLabelReducer and ExactLabelReducer
// Or: LabelReducer only takes data set of labels which should be mapped
// together and all it does is create the new labels and mappings
class LabelReducer {
    int num_labels;
    int num_reduced_labels;

    LabelSignature build_label_signature(const Label &label,
        const std::vector<bool> &var_is_used) const;
    EquivalenceRelation *compute_outside_equivalence(const Abstraction *abstraction,
                                                     const std::vector<Abstraction *> &all_abstractions,
                                                     const std::vector<const Label *> &labels) const;
public:
    LabelReducer(/*const std::vector<const Label *> &relevant_labels,*/
                 const std::vector<int> &abs_vars,
                 std::vector<const Label* > &labels);
    ~LabelReducer();
    void statistics() const;

    // exact label reduction
    LabelReducer(const Abstraction *abstraction,
                 const std::vector<Abstraction *> &all_abstractions,
                 std::vector<const Label* > &labels);
    void statistics2() const;

    int get_number_reduced_labels() const {
        return num_labels - num_reduced_labels;
    }
};

#endif
