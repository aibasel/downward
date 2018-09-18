#include "relaxation_heuristic.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/hash.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>

using namespace std;

namespace relaxation_heuristic {
// construction and destruction
RelaxationHeuristic::RelaxationHeuristic(const options::Options &opts)
    : Heuristic(opts) {

    VariablesProxy variables = task_proxy.get_variables();

    // Build proposition offsets.
    proposition_offsets.reserve(variables.size());
    int offset = 0;
    for (VariableProxy var : variables) {
        proposition_offsets.push_back(offset);
        offset += var.get_domain_size();
    }

    // Build propositions.
    propositions.reserve(task_properties::get_num_facts(task_proxy));
    int prop_id = 0;
    for (FactProxy fact : variables.get_facts()) {
        propositions.emplace_back(prop_id++);
    }

    // Build goal propositions.
    for (FactProxy goal : task_proxy.get_goals()) {
        Proposition *prop = get_proposition(goal);
        prop->is_goal = true;
        goal_propositions.push_back(prop);
    }

    // Build unary operators for operators and axioms.
    unary_operators.reserve(
        task_properties::get_num_total_effects(task_proxy));
    int op_no = 0;
    for (OperatorProxy op : task_proxy.get_operators())
        build_unary_operators(op, op_no++);
    for (OperatorProxy axiom : task_proxy.get_axioms())
        build_unary_operators(axiom, -1);

    // Simplify unary operators.
    utils::Timer simplify_timer;
    simplify();
    cout << "time to simplify: " << simplify_timer << endl;

    // Cross-reference unary operators.
    for (size_t i = 0; i < unary_operators.size(); ++i) {
        UnaryOperator *op = &unary_operators[i];
        for (size_t j = 0; j < op->precondition.size(); ++j)
            op->precondition[j]->precondition_of.push_back(op);
    }
}

RelaxationHeuristic::~RelaxationHeuristic() {
}

bool RelaxationHeuristic::dead_ends_are_reliable() const {
    return !task_properties::has_axioms(task_proxy);
}

const Proposition *RelaxationHeuristic::get_proposition(
    int var, int value) const {
    int offset = proposition_offsets[var];
    return &propositions[offset + value];
}

Proposition *RelaxationHeuristic::get_proposition(int var, int value) {
    int offset = proposition_offsets[var];
    return &propositions[offset + value];
}

Proposition *RelaxationHeuristic::get_proposition(const FactProxy &fact) {
    return get_proposition(fact.get_variable().get_id(), fact.get_value());
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

int RelaxationHeuristic::get_proposition_id(const Proposition &prop) const {
    int prop_no = &prop - propositions.data();
    assert(utils::in_bounds(prop_no, propositions));
    assert(prop_no == prop.id);
    return prop_no;
}

int RelaxationHeuristic::get_operator_id(const UnaryOperator &op) const {
    int op_no = &op - unary_operators.data();
    assert(utils::in_bounds(op_no, unary_operators));
    return op_no;
}

void RelaxationHeuristic::simplify() {
    /*
      Remove dominated unary operators, including duplicates.

      Unary operators with more than MAX_PRECONDITIONS_TO_TEST
      preconditions are ignored because we cannot handle them
      efficiently. (This is obviously an inelegant solution.)

      Apart from this restriction, operator o1 dominates operator o2 if:
      1. eff(o1) = eff(o2), and
      2. pre(o1) is a (not necessarily strict) subset of pre(o2), and
      3. cost(o1) <= cost(o2), and either
      4a. At least one of 2. and 3. is strict, or
      4b. id(o1) < id(o2).
      (Here, "id" is the position in the unary_operators vector.)

      This defines a strict partial order.
    */

    const int MAX_PRECONDITIONS_TO_TEST = 5;

    cout << "Simplifying " << unary_operators.size() << " unary operators..." << flush;

    /*
      TODO: This sorting code does not belong here, but simplify
      requires sorted preconditions to avoid missing simplification
      opportunities. It would be better to require the input to
      already be sorted and then assert that it is sorted.
    */
    for (UnaryOperator &op : unary_operators) {
        sort(op.precondition.begin(), op.precondition.end(),
             [&] (const Proposition *p1, const Proposition *p2) {
                 return get_proposition_id(*p1) <
                     get_proposition_id(*p2);
             });
    }

    /*
      First, we create a map that maps the preconditions and effect
      ("key") of each operator to its cost and index ("value").
      If multiple operators have the same key, the one with lowest
      cost wins. If this still results in a tie, the one with lowest
      index wins. These rules can be tested with a lexicographical
      comparison of the value.

      Note that for operators sharing the same preconditions and
      effect, our dominance relationship above is actually a strict
      *total* order (order by cost, then by id).

      For each key present in the data, the map stores the dominating
      element in this total order.
    */
    using Key = pair<vector<Proposition *>, Proposition *>;
    using Value = pair<int, int>;
    using Map = unordered_map<Key, Value>;
    Map unary_operator_index;
    unary_operator_index.reserve(unary_operators.size());

    for (size_t op_no = 0; op_no < unary_operators.size(); ++op_no) {
        const UnaryOperator &op = unary_operators[op_no];
        if (op.precondition.size() <= MAX_PRECONDITIONS_TO_TEST) {
            Key key(op.precondition, op.effect);
            Value value(op.base_cost, op_no);
            auto inserted = unary_operator_index.insert(
                make_pair(move(key), value));
            if (!inserted.second) {
                // We already had an element with this key; check its cost.
                Map::iterator iter = inserted.first;
                Value old_value = iter->second;
                if (value < old_value)
                    iter->second = value;
            }
        }
    }

    /*
      `dominating_key` is conceptually a local variable of `is_dominated`.
      We declare it outside to reduce vector reallocation overhead.
    */
    Key dominating_key;

    /*
      is_dominated: test if a given operator is dominated by an
      operator in the map.
    */
    auto is_dominated = [&](const UnaryOperator &op) {
        if (op.precondition.size() > MAX_PRECONDITIONS_TO_TEST)
            return false;

        /*
          Check all possible subsets X of pre(op) to see if there is a
          dominating operator with preconditions X represented in the
          map.
        */

        int op_no = get_operator_id(op);
        int cost = op.base_cost;

        const vector<Proposition *> &precondition = op.precondition;

        /*
          We handle the case X = pre(op) specially for efficiency and
          to ensure that an operator is not considered to be dominated
          by itself.

          From the discussion above that operators with the same
          precondition and effect are actually totally ordered, it is
          enough to test here whether looking up the key of op in the
          map results in an entry including op itself.
        */
        if (unary_operator_index[make_pair(precondition, op.effect)].second != op_no)
            return true;

        /*
          We now handle all cases where X is a strict subset of pre(op).
          Our map lookup ensures conditions 1. and 2., and because X is
          a strict subset, we also have 4a (which means we don't need 4b).
          So it only remains to check 3 for all hits.
        */
        vector<Proposition *> &dominating_precondition = dominating_key.first;
        dominating_key.second = op.effect;

        // We subtract "- 1" to generate all *strict* subsets of precondition.
        int powerset_size = (1 << precondition.size()) - 1;
        for (int mask = 0; mask < powerset_size; ++mask) {
            dominating_precondition.clear();
            for (size_t i = 0; i < precondition.size(); ++i)
                if (mask & (1 << i))
                    dominating_precondition.push_back(precondition[i]);
            Map::iterator found = unary_operator_index.find(dominating_key);
            if (found != unary_operator_index.end()) {
                Value dominator_value = found->second;
                int dominator_cost = dominator_value.first;
                if (dominator_cost <= cost)
                    return true;
            }
        }
        return false;
    };

    unary_operators.erase(
        remove_if(
            unary_operators.begin(),
            unary_operators.end(),
            is_dominated),
        unary_operators.end());

    cout << " done! [" << unary_operators.size() << " unary operators]" << endl;
}
}
