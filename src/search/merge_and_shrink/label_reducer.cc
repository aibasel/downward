#include "label_reducer.h"

#include "abstraction.h"
#include "equivalence_relation.h"

#include "../globals.h"
#include "../operator.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <ext/hash_map>
#include <iostream>

using namespace std;
using namespace __gnu_cxx;

void LabelReducer::reduce_labels(int abs_index,
                                 const vector<Abstraction *> &all_abstractions,
                                 std::vector<Label *> &labels,
                                 const LabelReduction &label_reduction,
                                 const vector<int> &variable_order) const {
    if (label_reduction == NONE) {
        exit_with(EXIT_CRITICAL_ERROR);
    }
    if (label_reduction == OLD) {
        // we need to normalize all abstraction to incorporate possible previous
        // label reductions (normalize cannot deal with several label reductions
        // at the time)
        for (size_t i = 0; i < all_abstractions.size(); ++i) {
            if (all_abstractions[i]) {
                all_abstractions[i]->normalize();
            }
        }
        reduce_old(all_abstractions[abs_index]->get_varset(), labels);
        return;
    }
    bool fixpoint = false;
    if (label_reduction == APPROXIMATIVE_WITH_FIXPOINT
            || label_reduction == EXACT_WITH_FIXPOINT) {
        fixpoint = true;
    }
    int current_abs_index = abs_index;
    int variable_order_index = 0;
    if (fixpoint) {
        assert(!variable_order.empty());
        while (variable_order[variable_order_index] != abs_index) {
            ++variable_order_index;
        }
        assert(variable_order[variable_order_index] == current_abs_index);
    }
    int index_of_last_unsuccesful_reduction = -1;
    vector<EquivalenceRelation *> local_equivalence_relations(all_abstractions.size(), 0);
    while (true) {
        Abstraction *current_abstraction = all_abstractions[current_abs_index];
        if (current_abstraction) {
            int reduced_labels = 0;
            if (label_reduction == EXACT || label_reduction == EXACT_WITH_FIXPOINT) {
                // Note: we need to normalize the current abstraction in order to
                // avoid that when normalizing it at some point later, we would
                // have two label reductions to incorporate.
                // See Abstraction::normalize()
                EquivalenceRelation *relation = compute_outside_equivalence(current_abs_index, all_abstractions,
                                                                            labels, local_equivalence_relations);
                reduced_labels = reduce_exactly(relation, labels);
                delete relation;
            } else {
                exit_with(EXIT_INPUT_ERROR);
            }
            if (!fixpoint) {
                break;
            }
            assert(fixpoint);
            assert(reduced_labels >= 0);
            if (reduced_labels > 0) {
                index_of_last_unsuccesful_reduction = -1;
            } else {
                assert(reduced_labels == 0);
                if (index_of_last_unsuccesful_reduction == -1) {
                    index_of_last_unsuccesful_reduction = variable_order_index;
                }
            }
        }
        // we can never end up here when *not* using fixpoint iteration
        assert(fixpoint);
        assert(!variable_order.empty());
        ++variable_order_index;
        if (variable_order_index == variable_order.size()) {
            variable_order_index = 0;
        }
        if (variable_order_index == index_of_last_unsuccesful_reduction) {
            // reached fixpoint
            break;
        }
        current_abs_index = variable_order[variable_order_index];
    }
    for (size_t i = 0; i < local_equivalence_relations.size(); ++i) {
        if (local_equivalence_relations[i]) {
            delete local_equivalence_relations[i];
        }
    }
}

typedef pair<int, int> Assignment;

struct LabelSignature {
    vector<int> data;

    LabelSignature(const vector<Assignment> &preconditions,
                   const vector<Assignment> &effects, int cost) {
        // We require that preconditions and effects are sorted by
        // variable -- some sort of canonical representation is needed
        // to guarantee that we can properly test for uniqueness.
        for (size_t i = 0; i < preconditions.size(); ++i) {
            if (i != 0)
                assert(preconditions[i].first > preconditions[i - 1].first);
            data.push_back(preconditions[i].first);
            data.push_back(preconditions[i].second);
        }
        data.push_back(-1); // marker
        for (size_t i = 0; i < effects.size(); ++i) {
            if (i != 0)
                assert(effects[i].first > effects[i - 1].first);
            data.push_back(effects[i].first);
            data.push_back(effects[i].second);
        }
        data.push_back(-1); // marker
        data.push_back(cost);
    }

    bool operator==(const LabelSignature &other) const {
        return data == other.data;
    }

    size_t hash() const {
        return ::hash_number_sequence(data, data.size());
    }
};

namespace __gnu_cxx {
template<>
struct hash<LabelSignature> {
    size_t operator()(const LabelSignature &sig) const {
        return sig.hash();
    }
};
}

LabelSignature LabelReducer::build_label_signature(
    const Label &label,
    const vector<bool> &var_is_used) const {
    vector<Assignment> preconditions;
    vector<Assignment> effects;

    const vector<Prevail> &prev = label.get_prevail();
    for (size_t i = 0; i < prev.size(); ++i) {
        int var = prev[i].var;
        if (var_is_used[var]) {
            int val = prev[i].prev;
            preconditions.push_back(make_pair(var, val));
        }
    }
    const vector<PrePost> &pre_post = label.get_pre_post();
    for (size_t i = 0; i < pre_post.size(); ++i) {
        int var = pre_post[i].var;
        if (var_is_used[var]) {
            int pre = pre_post[i].pre;
            if (pre != -1)
                preconditions.push_back(make_pair(var, pre));
            int post = pre_post[i].post;
            effects.push_back(make_pair(var, post));
        }
    }
    ::sort(preconditions.begin(), preconditions.end());
    ::sort(effects.begin(), effects.end());

    return LabelSignature(preconditions, effects, label.get_cost());
}

int LabelReducer::reduce_old(const vector<int> &abs_vars,
                             vector<Label *> &labels) const {
    int num_labels = 0;
    int num_labels_after_reduction = 0;

    vector<bool> var_is_used(g_variable_domain.size(), true);
    for (size_t i = 0; i < abs_vars.size(); ++i)
        var_is_used[abs_vars[i]] = false;

    hash_map<LabelSignature, vector<Label *> > reduced_label_map;
    // TODO: consider combining reduced_label_signature and is_label_reduced
    // into a set or hash-set (is_label_reduced only serves to make sure
    // that every label signature is pushed at most once into reduced_label_signatures).
    // The questions is if iterating over the set or hash set is efficient
    // (and produces the same result, because we would then very probably
    // settle for different 'canonical labels' because the ordering would be
    // lost).
    hash_map<LabelSignature, bool> is_label_reduced;
    vector<LabelSignature> reduced_label_signatures;

    for (size_t i = 0; i < labels.size(); ++i) {
        Label *label = labels[i];
        if (label->is_reduced()) {
            // ignore already reduced labels
            continue;
        }
        ++num_labels;
        LabelSignature signature = build_label_signature(
            *label, var_is_used);

        if (!reduced_label_map.count(signature)) {
            is_label_reduced[signature] = false;
            ++num_labels_after_reduction;
        } else {
            assert(is_label_reduced.count(signature));
            if (!is_label_reduced[signature]) {
                is_label_reduced[signature] = true;
                reduced_label_signatures.push_back(signature);
            }
        }
        reduced_label_map[signature].push_back(label);
    }
    assert(reduced_label_map.size() == num_labels_after_reduction);

    for (size_t i = 0; i < reduced_label_signatures.size(); ++i) {
        const LabelSignature &signature = reduced_label_signatures[i];
        const vector<Label *> &reduced_labels = reduced_label_map[signature];
        Label *new_label = new CompositeLabel(labels.size(), reduced_labels);
        labels.push_back(new_label);
    }

    cout << "Old, local label reduction: "
         << num_labels << " labels, "
         << num_labels_after_reduction << " after reduction"
         << endl;
    return num_labels - num_labels_after_reduction;
}

EquivalenceRelation *LabelReducer::compute_outside_equivalence(int abs_index,
                                                               const vector<Abstraction *> &all_abstractions,
                                                               const vector<Label *> &labels,
                                                               vector<EquivalenceRelation *> &local_equivalence_relations) const {
    /*Returns an equivalence relation over labels s.t. l ~ l'
    iff l and l' are locally equivalent in all transition systems
    T' \neq T. (They may or may not be locally equivalent in T.)
    Here: T = abstraction. */
    Abstraction *abstraction = all_abstractions[abs_index];
    assert(abstraction);
    //cout << abstraction->tag() << "compute combinable labels" << endl;
    abstraction->normalize();
    if (local_equivalence_relations[abs_index]) {
        delete local_equivalence_relations[abs_index];
        local_equivalence_relations[abs_index] = 0;
    }

    int num_labels = labels.size();
    vector<pair<int, int> > labeled_label_nos;
    labeled_label_nos.reserve(num_labels);
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        const Label *label = labels[label_no];
        assert(label->get_id() == label_no);
        if (label->is_reduced()) {
            // ignore already reduced labels
            continue;
        }
        labeled_label_nos.push_back(make_pair(0, label_no));
    }
    // start with the relation where all labels are equivalent
    EquivalenceRelation *relation = EquivalenceRelation::from_labels<int>(num_labels, labeled_label_nos);
    for (size_t i = 0; i < all_abstractions.size(); ++i) {
        Abstraction *abs = all_abstractions[i];
        if (!abs || abs == abstraction) {
            continue;
        }
        if (!abs->is_normalized()) {
            abs->normalize();
            if (local_equivalence_relations[i]) {
                delete local_equivalence_relations[i];
                local_equivalence_relations[i] = 0;
            }
        }
        //cout << abs->tag();
        if (!local_equivalence_relations[i]) {
            //cout << "compute local equivalence relation" << endl;
            local_equivalence_relations[i] = abs->compute_local_equivalence_relation();
        } else {
            //cout << "use cached local equivalence relation" << endl;
            assert(abs->is_normalized());
        }
        relation->refine(*local_equivalence_relations[i]);
    }
    return relation;
}

int LabelReducer::reduce_exactly(const EquivalenceRelation *relation, std::vector<Label *> &labels) const {
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (BlockListConstIter it = relation->begin(); it != relation->end(); ++it) {
        const Block &block = *it;
        vector<Label *> equivalent_labels;
        for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
            assert(*jt < labels.size());
            Label *label = labels[*jt];
            if (label->is_reduced()) {
                // ignore already reduced labels
                continue;
            }
            equivalent_labels.push_back(label);
            ++num_labels;
        }
        if (equivalent_labels.size() > 1) {
            Label *new_label = new CompositeLabel(labels.size(), equivalent_labels);
            labels.push_back(new_label);
        }
        if (!equivalent_labels.empty()) {
            ++num_labels_after_reduction;
        }
    }
    int number_reduced_labels = num_labels - num_labels_after_reduction;
    if (number_reduced_labels > 0) {
        cout << "Label reduction: "
             << num_labels << " labels, "
             << num_labels_after_reduction << " after reduction"
             << endl;
    }
    return number_reduced_labels;
}
