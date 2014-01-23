#include "label_reducer.h"

#include "abstraction.h"
#include "equivalence_relation.h"

#include "../globals.h"
#include "../operator.h"
#include "../utilities.h"

#include <cassert>
#include <ext/hash_map>
#include <iostream>

using namespace std;
using namespace __gnu_cxx;

LabelReducer::LabelReducer(int abs_index,
                           const vector<Abstraction *> &all_abstractions,
                           vector<const Label *> &labels,
                           bool exact,
                           bool fixpoint) {
    if (exact) {
        int current_index = abs_index;
        num_reduced_labels = 0;
        while (true) {
            Abstraction *current_abstraction = all_abstractions[current_index];
            if (current_abstraction) {
                // Note: we need to normalize the current abstraction in order to
                // avoid that when normalizing it at some point later, we would
                // have two label reductions to incorporate.
                // See Abstraction::normalize()
                current_abstraction->normalize();
                EquivalenceRelation *relation = compute_outside_equivalence(current_abstraction, all_abstractions, labels);
                int reduced_labels = reduce_exactly(relation, labels);
                delete relation;
                num_reduced_labels += reduced_labels;
                if (reduced_labels == 0 || !fixpoint) {
                    break;
                }
            }
            ++current_index;
            if (current_index == all_abstractions.size()) {
                current_index = 0;
            }
        }
    } else {
        for (size_t i = 0; i < all_abstractions.size(); ++i) {
            if (all_abstractions[i]) {
                all_abstractions[i]->normalize();
            }
        }
        num_reduced_labels = reduce_approximatively(all_abstractions[abs_index]->get_varset(), labels);
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

int LabelReducer::reduce_approximatively(const vector<int> &abs_vars,
                                         vector<const Label *> &labels) const {
    int num_labels = 0;//relevant_labels.size();
    int num_labels_after_reduction = 0;

    vector<bool> var_is_used(g_variable_domain.size(), true);
    for (size_t i = 0; i < abs_vars.size(); ++i)
        var_is_used[abs_vars[i]] = false;

    hash_map<LabelSignature, vector<const Label *> > reduced_label_map;
    // TODO: consider combining reduced_label_signature and is_label_reduced
    // into a set or hash-set (is_label_reduced only serves to make sure
    // that every label signature is pushed at most once into reduced_label_signatures).
    // The questions is if iterating over the set or hash set is efficient
    // (and produces the same result, because we would then very probably
    // settle for different 'canonical labels' because the ordering would be
    // lost).
    hash_map<LabelSignature, bool> is_label_reduced;
    vector<LabelSignature> reduced_label_signatures;

    //for (size_t i = 0; i < relevant_labels.size(); ++i) {
        //const Label *label = relevant_labels[i];
    for (size_t i = 0; i < labels.size(); ++i) {
        const Label *label = labels[i];
        if (label->get_reduced_label() != label) {
            // ignore already reduced labels
            continue;
        }
        ++num_labels;
        // require that the considered abstraction's relevant labels are reduced
        // to make sure that we cannot reduce the same label several times.
        // TODO: does this assertion currently hold?
        assert(label->get_reduced_label() == label);
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
        const vector<const Label *> &reduced_labels = reduced_label_map[signature];
        const Label *new_label = new CompositeLabel(labels.size(), reduced_labels);
        labels.push_back(new_label);
    }

    cout << "Label reduction: "
         << num_labels << " labels, "
         << num_labels_after_reduction << " after reduction"
         << endl;
    return num_labels - num_labels_after_reduction;
}

EquivalenceRelation *LabelReducer::compute_outside_equivalence(const Abstraction *abstraction,
                                                               const vector<Abstraction *> &all_abstractions,
                                                               const vector<const Label *> &labels) const {
    /*Returns an equivalence relation over labels s.t. l ~ l'
    iff l and l' are locally equivalent in all transition systems
    T' \neq T. (They may or may not be locally equivalent in T.)
    Here: T = abstraction. */
    cout << "compute outside equivalence for " << abstraction->tag() << endl;

    int num_labels = labels.size();
    vector<pair<int, int> > labeled_label_nos;
    labeled_label_nos.reserve(num_labels);
    for (int label_no = 0; label_no < num_labels; ++label_no) {
        const Label *label = labels[label_no];
        assert(label->get_id() == label_no);
        if (label->get_reduced_label() != label) {
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
        cout << "computing local equivalence for " << abs->tag() << endl;
        if (!abs->is_normalized()) {
            // TODO: get rid of this, as normalize itself checks whether
            // the abstraction is already normalized or not?
            cout << "need to normalize" << endl;
            abs->normalize();
        }
        EquivalenceRelation *next_relation = abs->compute_local_equivalence_relation();
        relation->refine(*next_relation);
        delete next_relation;
    }
    return relation;
}

int LabelReducer::reduce_exactly(const EquivalenceRelation *relation, vector<const Label *> &labels) const {
    int num_labels = 0;
    int num_labels_after_reduction = 0;
    for (BlockListConstIter it = relation->begin(); it != relation->end(); ++it) {
        const Block &block = *it;
        vector<const Label *> equivalent_labels;
        for (ElementListConstIter jt = block.begin(); jt != block.end(); ++jt) {
            assert(*jt < labels.size());
            const Label *label = labels[*jt];
            if (label->get_reduced_label() != label) {
                // ignore already reduced labels
                continue;
            }
            equivalent_labels.push_back(label);
            ++num_labels;
        }
        if (equivalent_labels.size() > 1) {
            const Label *new_label = new CompositeLabel(labels.size(), equivalent_labels);
            labels.push_back(new_label);
        }
        if (!equivalent_labels.empty()) {
            ++num_labels_after_reduction;
        }
    }
    cout << "Label reduction: "
         << num_labels << " labels, "
         << num_labels_after_reduction << " after reduction"
         << endl;
    return num_labels - num_labels_after_reduction;
}
