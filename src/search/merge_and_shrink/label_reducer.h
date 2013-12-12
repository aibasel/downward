#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include "../globals.h"
#include "../operator.h"
#include "../operator_cost.h"
#include "label.h"

#include <cassert>
#include <vector>

class LabelSignature;

class LabelReducer {
    // TODO: remove this instance variables and print them directly at the
    // end of the constructor (and get rid of statistics())?
    int num_pruned_vars;
    int num_labels;
    int num_reduced_labels;

    LabelSignature build_label_signature(const Label &label,
        const std::vector<bool> &var_is_used) const;
public:
    LabelReducer(const std::vector<const Label *> &relevant_labels,
                 const std::vector<int> &abs_vars,
                 std::vector<const Label* > &labels);
    ~LabelReducer();

    void statistics() const;
};

#endif
