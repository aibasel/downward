#include "label_reducer.h"

#include "../globals.h"
#include "../operator.h"
#include "../utilities.h"

#include <cassert>
#include <ext/hash_map>
#include <iostream>

using namespace std;
using namespace __gnu_cxx;

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

LabelReducer::LabelReducer(const vector<const Label *> &relevant_labels,
    const vector<int> &pruned_vars, std::vector<const Label *> &labels) {
    num_pruned_vars = pruned_vars.size();
    num_labels = relevant_labels.size();
    num_reduced_labels = 0;

    vector<bool> var_is_used(g_variable_domain.size(), true);
    for (size_t i = 0; i < pruned_vars.size(); ++i)
        var_is_used[pruned_vars[i]] = false;

    hash_map<LabelSignature, const Label *> reduced_label_map;
    hash_map<LabelSignature, bool> is_label_reduced;
    reduced_label_by_index.resize(labels.size(), 0);

    for (size_t i = 0; i < relevant_labels.size(); ++i) {
        const Label *label = relevant_labels[i];
        LabelSignature signature = build_label_signature(
            *label, var_is_used);

        int label_index = label->get_index();
        if (!reduced_label_map.count(signature)) {
            reduced_label_map[signature] = label;
            is_label_reduced[signature] = false;
            reduced_label_by_index[label_index] = label;
            ++num_reduced_labels;
        } else {
            assert(is_label_reduced.count(signature));
            if (!is_label_reduced[signature]) {
                is_label_reduced[signature] = true;
                const Label *old_label = reduced_label_map[signature];
                int old_label_index = old_label->get_index();
                assert(reduced_label_by_index[old_label_index] == old_label);
                // create new label from old label as in reduced_label_map
                const Label *new_label = new CompositeLable(labels.size(), old_label);
                labels.push_back(new_label);
                reduced_label_map[signature] = new_label;
                reduced_label_by_index[old_label_index] = new_label;
            }
            reduced_label_map[signature]->add_mapping_label(label);
            reduced_label_by_index[label_index] = reduced_label_map[signature];
        }
    }
    assert(reduced_label_map.size() == num_reduced_labels);
}

LabelReducer::~LabelReducer() {
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

void LabelReducer::statistics() const {
    cout << "Label reduction: "
         << num_pruned_vars << " pruned vars, "
         << num_labels << " labels, "
         << num_reduced_labels << " reduced labels"
         << endl;
}
