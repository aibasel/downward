#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include <vector>

class Abstraction;
class EquivalenceRelation;
class Label;
class LabelSignature;
class Options;

class LabelReducer {
    /* none: no label reduction will be performed

       old: emulate the label reduction as desribed in the
       IJCAI 2011 paper by Nissim, Hoffmann and Helmert.

       two_abstractions: compute the 'combinable relation'
       for labels only for the two abstractions that will
       be merged next and reduce labels.

       all_abstractions: compute the 'combinable relation'
       for labels once for every abstraction and reduce
       labels.

       all_abstractions_with_fixpoint: keep computing the
       'combinable relation' for labels iteratively for all
       abstractions until no more labels can be reduced.
     */
    enum LabelReductionMethod {
        NONE,
        OLD,
        TWO_ABSTRACTIONS,
        ALL_ABSTRACTIONS,
        ALL_ABSTRACTIONS_WITH_FIXPOINT
    };

    enum LabelReductionSystemOrder {
        REGULAR,
        REVERSE,
        RANDOM
    };

    LabelReductionMethod label_reduction_method;
    LabelReductionSystemOrder label_reduction_system_order;
    std::vector<std::size_t> system_order;

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
    void reduce_labels(std::pair<int, int> next_merge,
                       const std::vector<Abstraction *> &all_abstractions,
                       std::vector<Label* > &labels) const;
    void dump_options() const;
};

#endif
