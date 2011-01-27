#include "globals.h"
#include "operator.h"
#include "raz_operator_registry.h"

#include <cassert>
#include <ext/hash_map>
#include <iostream>

using namespace std;
using namespace __gnu_cxx;

typedef pair<int, int> Assignment;

struct OperatorSignature {
    vector<int> data;

    OperatorSignature(const vector<Assignment> &preconditions, const vector<
                          Assignment> &effects, const int cost) {
        // We require that preconditions and effects are sorted by
        // variable -- some sort of canonical representation is needed
        // to guarantee that we can properly test for uniqueness.
        for (int i = 0; i < preconditions.size(); i++) {
            if (i != 0)
                assert(preconditions[i].first > preconditions[i - 1].first);
            data.push_back(preconditions[i].first);
            data.push_back(preconditions[i].second);
        }
        data.push_back(-1);         // marker
        for (int i = 0; i < effects.size(); i++) {
            if (i != 0)
                assert(effects[i].first > effects[i - 1].first);
            data.push_back(effects[i].first);
            data.push_back(effects[i].second);
        }
        data.push_back(-1);         // marker
        data.push_back(cost);
    }

    bool operator==(const OperatorSignature &other) const {
        return data == other.data;
    }

    size_t hash() const {
        // HACK! This is copied from state.cc and should be factored
        // out to a common place.
        size_t hash_value = 0x345678;
        size_t mult = 1000003;
        for (int i = data.size() - 1; i >= 0; i--) {
            hash_value = (hash_value ^ data[i]) * mult;
            mult += 82520 + i + i;
        }
        hash_value += 97531;
        return hash_value;
    }
};

namespace __gnu_cxx {
template<>
struct hash<OperatorSignature> {
    size_t operator()(const OperatorSignature &sig) const {
        return sig.hash();
    }
};
}


OperatorRegistry::OperatorRegistry(
    const vector<const Operator *> &relevant_operators,
    const vector<int> &pruned_vars) {
    num_vars = pruned_vars.size();
    num_operators = relevant_operators.size();
    num_canonical_operators = 0;

    vector<int> var_is_used(g_variable_domain.size(), true);
    for (int i = 0; i < pruned_vars.size(); i++)
        var_is_used[pruned_vars[i]] = false;

    hash_map<OperatorSignature, const Operator *> canonical_op_map;
    canonical_operators.resize(g_operators.size(), 0);

    for (int i = 0; i < relevant_operators.size(); i++) {
        const Operator *op = relevant_operators[i];
        vector<Assignment> preconditions;
        vector<Assignment> effects;
        int op_cost = op->get_cost();
        const vector<Prevail> &prev = op->get_prevail();
        for (int j = 0; j < prev.size(); j++) {
            int var = prev[j].var;
            if (var_is_used[var]) {
                int val = prev[j].prev;
                preconditions.push_back(make_pair(var, val));
            }
        }
        const vector<PrePost> &pre_post = op->get_pre_post();
        for (int j = 0; j < pre_post.size(); j++) {
            int var = pre_post[j].var;
            if (var_is_used[var]) {
                int pre = pre_post[j].pre;
                if (pre != -1)
                    preconditions.push_back(make_pair(var, pre));
                int post = pre_post[j].post;
                effects.push_back(make_pair(var, post));
            }
        }
        ::sort(preconditions.begin(), preconditions.end());
        ::sort(effects.begin(), effects.end());

        OperatorSignature op_sig(preconditions, effects, op_cost);      //(Raz) - only reducing labels with the same cost!
        int op_index = get_op_index(op);
        if (!canonical_op_map.count(op_sig)) {
            canonical_op_map[op_sig] = op;
            canonical_operators[op_index] = op;
            num_canonical_operators++;
        } else {
            canonical_operators[op_index] = canonical_op_map[op_sig];
        }
    }
    assert(canonical_op_map.size() == num_canonical_operators);
}

OperatorRegistry::~OperatorRegistry() {
}

void OperatorRegistry::statistics() const {
    cout << "operator registry: " << num_vars << " vars, " << num_operators
         << " operators, " << num_canonical_operators
         << " canonical operators" << endl;
}
