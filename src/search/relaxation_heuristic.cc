#include "relaxation_heuristic.h"

#include "globals.h"
#include "operator.h"
#include "state.h"

#include <cassert>
#include <vector>
using namespace std;

#include <ext/hash_map>
using namespace __gnu_cxx;

// construction and destruction
RelaxationHeuristic::RelaxationHeuristic(const Options &opts)
    : Heuristic(opts) {
}

RelaxationHeuristic::~RelaxationHeuristic() {
}

// initialization
void RelaxationHeuristic::initialize() {
    // Build propositions.
    int prop_id = 0;
    propositions.resize(g_variable_domain.size());
    for (int var = 0; var < g_variable_domain.size(); var++) {
        for (int value = 0; value < g_variable_domain[var]; value++)
            propositions[var].push_back(Proposition(prop_id++));
    }

    // Build goal propositions.
    for (int i = 0; i < g_goal.size(); i++) {
        int var = g_goal[i].first, val = g_goal[i].second;
        propositions[var][val].is_goal = true;
        goal_propositions.push_back(&propositions[var][val]);
    }

    // Build unary operators for operators and axioms.
    for (int i = 0; i < g_operators.size(); i++)
        build_unary_operators(g_operators[i], i);
    for (int i = 0; i < g_axioms.size(); i++)
        build_unary_operators(g_axioms[i], -1);

    // Simplify unary operators.
    simplify();

    // Cross-reference unary operators.
    for (int i = 0; i < unary_operators.size(); i++) {
        UnaryOperator *op = &unary_operators[i];
        for (int j = 0; j < op->precondition.size(); j++)
            op->precondition[j]->precondition_of.push_back(op);
    }
}

void RelaxationHeuristic::build_unary_operators(const Operator &op, int op_no) {
    int base_cost = get_adjusted_cost(op);
    const vector<Prevail> &prevail = op.get_prevail();
    const vector<PrePost> &pre_post = op.get_pre_post();
    vector<Proposition *> precondition;
    for (int i = 0; i < prevail.size(); i++) {
        assert(prevail[i].var >= 0 && prevail[i].var < g_variable_domain.size());
        assert(prevail[i].prev >= 0 && prevail[i].prev < g_variable_domain[prevail[i].var]);
        precondition.push_back(&propositions[prevail[i].var][prevail[i].prev]);
    }
    for (int i = 0; i < pre_post.size(); i++) {
        if (pre_post[i].pre != -1) {
            assert(pre_post[i].var >= 0 && pre_post[i].var < g_variable_domain.size());
            assert(pre_post[i].pre >= 0 && pre_post[i].pre < g_variable_domain[pre_post[i].var]);
            precondition.push_back(&propositions[pre_post[i].var][pre_post[i].pre]);
        }
    }
    for (int i = 0; i < pre_post.size(); i++) {
        assert(pre_post[i].var >= 0 && pre_post[i].var < g_variable_domain.size());
        assert(pre_post[i].post >= 0 && pre_post[i].post < g_variable_domain[pre_post[i].var]);
        Proposition *effect = &propositions[pre_post[i].var][pre_post[i].post];
        const vector<Prevail> &eff_cond = pre_post[i].cond;
        for (int j = 0; j < eff_cond.size(); j++) {
            assert(eff_cond[j].var >= 0 && eff_cond[j].var < g_variable_domain.size());
            assert(eff_cond[j].prev >= 0 && eff_cond[j].prev < g_variable_domain[eff_cond[j].var]);
            precondition.push_back(&propositions[eff_cond[j].var][eff_cond[j].prev]);
        }
        unary_operators.push_back(UnaryOperator(precondition, effect, op_no, base_cost));
        precondition.erase(precondition.end() - eff_cond.size(), precondition.end());
    }
}

class hash_unary_operator {
public:
    size_t operator()(const pair<vector<Proposition *>, Proposition *> &key) const {
        // NOTE: We used to hash the Proposition* values directly, but
        // this had the disadvantage that the results were not
        // reproducible. This propagates through to the heuristic
        // computation: runs on different computers could lead to
        // different initial h values, for example.

        unsigned long hash_value = key.second->id;
        const vector<Proposition *> &vec = key.first;
        for (int i = 0; i < vec.size(); i++)
            hash_value = 17 * hash_value + vec[i]->id;
        return size_t(hash_value);
    }
};


static bool compare_prop_pointer(const Proposition *p1, const Proposition *p2) {
    return p1->id < p2->id;
}


void RelaxationHeuristic::simplify() {
    // Remove duplicate or dominated unary operators.

    /*
      Algorithm: Put all unary operators into a hash_map
      (key: condition and effect; value: index in operator vector.
      This gets rid of operators with identical conditions.

      Then go through the hash_map, checking for each element if
      none of the possible dominators are part of the hash_map.
      Put the element into the new operator vector iff this is the case.

      In both loops, be careful to ensure that a higher-cost operator
      never dominates a lower-cost operator.
    */


    cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

    typedef pair<vector<Proposition *>, Proposition *> HashKey;
    typedef hash_map<HashKey, int, hash_unary_operator> HashMap;
    HashMap unary_operator_index;
    unary_operator_index.resize(unary_operators.size() * 2);

    for (int i = 0; i < unary_operators.size(); i++) {
        UnaryOperator &op = unary_operators[i];
        sort(op.precondition.begin(), op.precondition.end(), compare_prop_pointer);
        HashKey key(op.precondition, op.effect);
        pair<HashMap::iterator, bool> inserted = unary_operator_index.insert(
            make_pair(key, i));
        if (!inserted.second) {
            // We already had an element with this key; check its cost.
            HashMap::iterator iter = inserted.first;
            int old_op_no = iter->second;
            int old_cost = unary_operators[old_op_no].base_cost;
            int new_cost = unary_operators[i].base_cost;
            if (new_cost < old_cost)
                iter->second = i;
            assert(unary_operators[unary_operator_index[key]].base_cost ==
                   min(old_cost, new_cost));
        }
    }

    vector<UnaryOperator> old_unary_operators;
    old_unary_operators.swap(unary_operators);

    for (HashMap::iterator it = unary_operator_index.begin();
         it != unary_operator_index.end(); ++it) {
        const HashKey &key = it->first;
        int unary_operator_no = it->second;
        int powerset_size = (1 << key.first.size()) - 1; // -1: only consider proper subsets
        bool match = false;
        if (powerset_size <= 31) { // HACK! Don't spend too much time here...
            for (int mask = 0; mask < powerset_size; mask++) {
                HashKey dominating_key = make_pair(vector<Proposition *>(), key.second);
                for (int i = 0; i < key.first.size(); i++)
                    if (mask & (1 << i))
                        dominating_key.first.push_back(key.first[i]);
                HashMap::iterator found = unary_operator_index.find(
                    dominating_key);
                if (found != unary_operator_index.end()) {
                    int my_cost = old_unary_operators[unary_operator_no].base_cost;
                    int dominator_op_no = found->second;
                    int dominator_cost = old_unary_operators[dominator_op_no].base_cost;
                    if (dominator_cost <= my_cost) {
                        match = true;
                        break;
                    }
                }
            }
        }
        if (!match)
            unary_operators.push_back(old_unary_operators[unary_operator_no]);
    }

    cout << " done! [" << unary_operators.size() << " unary operators]" << endl;
}
