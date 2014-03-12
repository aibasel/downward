#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include <vector>

class Abstraction;
class EquivalenceRelation;
class Label;
class LabelSignature;
class Options;

enum LabelReductionMethod {
    NONE,
    OLD,
    ONE_ABSTRACTION,
    ALL_ABSTRACTIONS,
    ALL_ABSTRACTIONS_WITH_FIXPOINT
};

enum FixpointVariableOrder {
    REGULAR,
    REVERSE,
    RANDOM
};

class LabelReducer {
    LabelReductionMethod label_reduction_method;
    FixpointVariableOrder fixpoint_variable_order;
    std::vector<int> variable_order;

    // old label reduction
    LabelSignature build_label_signature(const Label &label,
        const std::vector<bool> &var_is_used) const;
    // returns true iff at least one new label has been created
    bool reduce_old(const std::vector<int> &abs_vars,
                    std::vector<Label *> &labels) const;

    // exact label reduction
    EquivalenceRelation *compute_outside_equivalence(int abs_index,
                                                     const std::vector<Abstraction *> &all_abstractions,
                                                     const std::vector<Label *> &labels,
                                                     std::vector<EquivalenceRelation *> &local_equivalence_relations) const;
    // returns true iff at least one new label has been created
    bool reduce_exactly(const EquivalenceRelation *relation,
                        std::vector<Label *> &labels) const;
public:
    explicit LabelReducer(const Options &options);
    ~LabelReducer() {}
    void reduce_labels(int abs_index,
                       const std::vector<Abstraction *> &all_abstractions,
                       std::vector<Label* > &labels) const;
    void dump_options() const;
};

#endif
