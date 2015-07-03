#include "relaxation_heuristic.h"

#include "global_operator.h"
#include "global_state.h"
#include "globals.h"
#include "task_proxy.h"
#include "utilities.h"
#include "utilities_hash.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <vector>

using namespace std;

// construction and destruction
RelaxationHeuristic::RelaxationHeuristic(const Options &opts)
    : Heuristic(opts) {
}

RelaxationHeuristic::~RelaxationHeuristic() {
}

bool RelaxationHeuristic::dead_ends_are_reliable() const {
    return !has_axioms();
}

// initialization
void RelaxationHeuristic::initialize() {
    // Build propositions.
    int prop_id = 0;
    VariablesProxy variables = task_proxy.get_variables();
    propositions.resize(variables.size());
    for (FactProxy fact : variables.get_facts()) {
        propositions[fact.get_variable().get_id()].push_back(Proposition(prop_id++));
    }

    // Build goal propositions.
    for (FactProxy goal : task_proxy.get_goals()) {
        Proposition *prop = get_proposition(goal);
        prop->is_goal = true;
        goal_propositions.push_back(prop);
    }

    // Build unary operators for operators and axioms.
    int op_no = 0;
    for (OperatorProxy op : task_proxy.get_operators())
        build_unary_operators(op, op_no++);
    for (OperatorProxy axiom : task_proxy.get_axioms())
        build_unary_operators(axiom, -1);

    // Simplify unary operators.
    simplify();

    // Cross-reference unary operators.
    for (size_t i = 0; i < unary_operators.size(); ++i) {
        UnaryOperator *op = &unary_operators[i];
        for (size_t j = 0; j < op->precondition.size(); ++j)
            op->precondition[j]->precondition_of.push_back(op);
    }
}

Proposition *RelaxationHeuristic::get_proposition(const FactProxy &fact) {
    int var = fact.get_variable().get_id();
    int value = fact.get_value();
    assert(in_bounds(var, propositions));
    assert(in_bounds(value, propositions[var]));
    return &propositions[var][value];
}

void RelaxationHeuristic::build_unary_operators(const OperatorProxy &op, int op_no) {
    int base_cost = op.get_cost();
    vector<Proposition *> precondition_props;
    for (FactProxy precondition : op.get_preconditions()) {
        precondition_props.push_back(get_proposition(precondition));
    }
    for (EffectProxy effect : op.get_effects()) {
        Proposition *effect_prop = get_proposition(effect.get_fact());
        EffectConditionsProxy eff_conds = effect.get_conditions();
        for (FactProxy eff_cond : eff_conds) {
            precondition_props.push_back(get_proposition(eff_cond));
        }
        unary_operators.push_back(UnaryOperator(precondition_props, effect_prop, op_no, base_cost));
        precondition_props.erase(precondition_props.end() - eff_conds.size(), precondition_props.end());
    }
}

void RelaxationHeuristic::simplify() {
    // Remove duplicate or dominated unary operators.

    /*
      Algorithm: Put all unary operators into a map
      (key: condition and effect; value: index in operator vector.
      This gets rid of operators with identical conditions.

      Then go through the map, checking for each element if
      none of the possible dominators are part of the map.
      Put the element into the new operator vector iff this is the case.

      In both loops, be careful to ensure that a higher-cost operator
      never dominates a lower-cost operator.
    */


    cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

    typedef pair<vector<Proposition *>, Proposition *> Key;
    typedef map<Key, int> Map;
    Map unary_operator_index;

    for (size_t i = 0; i < unary_operators.size(); ++i) {
        UnaryOperator &op = unary_operators[i];
        sort(op.precondition.begin(), op.precondition.end(),
             [] (const Proposition * p1, const Proposition * p2) {
                 return p1->id < p2->id;
             }
             );
        Key key(op.precondition, op.effect);
        pair<Map::iterator, bool> inserted = unary_operator_index.insert(
            make_pair(key, i));
        if (!inserted.second) {
            // We already had an element with this key; check its cost.
            Map::iterator iter = inserted.first;
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

    for (Map::iterator it = unary_operator_index.begin();
         it != unary_operator_index.end(); ++it) {
        const Key &key = it->first;
        int unary_operator_no = it->second;
        bool match = false;
        if (key.first.size() <= 5) { // HACK! Don't spend too much time here...
            int powerset_size = (1 << key.first.size()) - 1; // -1: only consider proper subsets
            for (int mask = 0; mask < powerset_size; ++mask) {
                Key dominating_key = make_pair(vector<Proposition *>(), key.second);
                for (size_t i = 0; i < key.first.size(); ++i)
                    if (mask & (1 << i))
                        dominating_key.first.push_back(key.first[i]);
                Map::iterator found = unary_operator_index.find(
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
