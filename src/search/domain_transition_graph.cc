#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <unordered_map>

using namespace std;

#include "domain_transition_graph.h"
#include "global_operator.h"
#include "globals.h"
#include "utilities.h"
#include "utilities_hash.h"


void DomainTransitionGraph::read_all(istream &in) {
    int var_count = g_variable_domain.size();

    // First step: Allocate graphs and nodes.
    g_transition_graphs.reserve(var_count);
    for (int var = 0; var < var_count; ++var) {
        int range = g_variable_domain[var];
        DomainTransitionGraph *dtg = new DomainTransitionGraph(var, range);
        g_transition_graphs.push_back(dtg);
    }

    // Second step: Read transitions from file.
    for (int var = 0; var < var_count; ++var)
        g_transition_graphs[var]->read_data(in);

    // Third step: Simplify transitions.
    // Don't do this for ADL domains, because the algorithm is exponential
    // in the number of conditions of a transition, which is a constant for STRIPS
    // domains, but not for ADL domains.

    cout << "Simplifying transitions..." << flush;
    for (int var = 0; var < var_count; ++var) {
        vector<ValueNode> &nodes = g_transition_graphs[var]->nodes;
        for (size_t value = 0; value < nodes.size(); ++value)
            for (size_t i = 0; i < nodes[value].transitions.size(); ++i)
                nodes[value].transitions[i].simplify();
    }
    cout << " done!" << endl;
}

DomainTransitionGraph::DomainTransitionGraph(int var_index, int node_count) {
    is_axiom = g_axiom_layers[var_index] != -1;
    var = var_index;
    nodes.reserve(node_count);
    for (int value = 0; value < node_count; ++value)
        nodes.push_back(ValueNode(this, value));
    last_helpful_transition_extraction_time = -1;
}

void DomainTransitionGraph::read_data(istream &in) {
    check_magic(in, "begin_DTG");

    map<int, int> global_to_local_child;
    map<int, int> global_to_cea_parent;
    map<pair<int, int>, int> transition_index;
    // TODO: This transition index business is caused by the fact
    //       that transitions in the input are not grouped by target
    //       like they should be. Change this.

    for (size_t origin = 0; origin < nodes.size(); ++origin) {
        int trans_count;
        in >> trans_count;
        for (int i = 0; i < trans_count; ++i) {
            int target, operator_index;
            in >> target;
            in >> operator_index;

            pair<int, int> arc = make_pair(origin, target);
            if (!transition_index.count(arc)) {
                transition_index[arc] = nodes[origin].transitions.size();
                nodes[origin].transitions.push_back(ValueTransition(&nodes[target]));
            }

            assert(transition_index.count(arc));
            ValueTransition *transition = &nodes[origin].transitions[transition_index[arc]];

            vector<LocalAssignment> precond;
            vector<LocalAssignment> cea_precond;
            vector<LocalAssignment> no_effect;
            vector<LocalAssignment> cea_effect;
            int precond_count;
            in >> precond_count;

            vector<pair<int, int> > precond_pairs; // Needed to build up cea_effect.
            for (int j = 0; j < precond_count; ++j) {
                int global_var, val;
                in >> global_var >> val;
                precond_pairs.push_back(make_pair(global_var, val));

                // Processing for pruned DTG.
                if (global_var < var) { // [ignore cycles]
                    if (!global_to_local_child.count(global_var)) {
                        global_to_local_child[global_var] = local_to_global_child.size();
                        local_to_global_child.push_back(global_var);
                    }
                    int local_var = global_to_local_child[global_var];
                    precond.push_back(LocalAssignment(local_var, val));
                }

                // Processing for full DTG (cea CG).
                if (!global_to_cea_parent.count(global_var)) {
                    global_to_cea_parent[global_var] = cea_parents.size();
                    cea_parents.push_back(global_var);
                }
                int cea_parent = global_to_cea_parent[global_var];
                cea_precond.push_back(LocalAssignment(cea_parent, val));
            }
            GlobalOperator *the_operator;
            if (is_axiom) {
                assert(in_bounds(operator_index, g_axioms));
                the_operator = &g_axioms[operator_index];
            } else {
                assert(in_bounds(operator_index, g_operators));
                the_operator = &g_operators[operator_index];
            }

            // Build up cea_effect. This is messy because this isn't
            // really the place to do this, and it was added very much as an
            // afterthought.
            sort(precond_pairs.begin(), precond_pairs.end());

            unordered_map<int, int> pre_map;
            const vector<GlobalCondition> &preconditions = the_operator->get_preconditions();
            for (size_t j = 0; j < preconditions.size(); ++j)
                pre_map[preconditions[j].var] = preconditions[j].val;

            const vector<GlobalEffect> &effects = the_operator->get_effects();
            for (size_t j = 0; j < effects.size(); ++j) {
                int var_no = effects[j].var;
                int pre = -1;
                auto pre_it = pre_map.find(var_no);
                if (pre_it != pre_map.end())
                    pre = pre_it->second;
                int post = effects[j].val;

                if (var_no == var || !global_to_cea_parent.count(var_no)) {
                    // This is either an effect on the variable we're
                    // building the DTG for, or an effect on a variable we
                    // don't need to track because it doesn't appear in
                    // conditions of this DTG. Ignore it.
                    continue;
                }

                vector<pair<int, int> > triggercond_pairs;
                if (pre != -1)
                    triggercond_pairs.push_back(make_pair(var_no, pre));

                const vector<GlobalCondition> &cond = effects[j].conditions;
                for (size_t k = 0; k < cond.size(); ++k)
                    triggercond_pairs.push_back(make_pair(cond[k].var, cond[k].val));
                sort(triggercond_pairs.begin(), triggercond_pairs.end());

                if (includes(precond_pairs.begin(), precond_pairs.end(),
                             triggercond_pairs.begin(), triggercond_pairs.end())) {
                    int cea_parent = global_to_cea_parent[var_no];
                    cea_effect.push_back(LocalAssignment(cea_parent, post));
                }
            }

#define TRACK_SIDE_EFFECTS 1
#if !TRACK_SIDE_EFFECTS
            cea_effect.clear();
#endif

            transition->labels.push_back(
                ValueTransitionLabel(the_operator, precond, no_effect));
            transition->cea_labels.push_back(
                ValueTransitionLabel(the_operator, cea_precond, cea_effect));
        }
    }
    check_magic(in, "end_DTG");
}

void DomainTransitionGraph::dump() const {
    cout << "DomainTransitionGraph::dump() not implemented." << endl;
    assert(false);
}

void DomainTransitionGraph::get_successors(int value, vector<int> &result) const {
    assert(result.empty());
    assert(in_bounds(value, nodes));
    const vector<ValueTransition> &transitions = nodes[value].transitions;
    result.reserve(transitions.size());
    for (size_t i = 0; i < transitions.size(); ++i)
        result.push_back(transitions[i].target->value);
}

void ValueTransition::simplify() {
    simplify_labels(labels);
    simplify_labels(cea_labels);
}

void ValueTransition::simplify_labels(
    vector<ValueTransitionLabel> &label_vec) {
    // Remove labels with duplicate or dominated conditions.

    /*
      Algorithm: Put all transitions into an unordered_map
      (key: condition, value: index in label list).
      This already gets rid of transitions with identical conditions.

      Then go through the unordered_map, checking for each element if
      none of the subset conditions are part of the unordered_map.
      Put the element into the new labels list iff this is the case.
     */

    typedef vector<pair<int, int> > HashKey;
    typedef unordered_map<HashKey, int> HashMap;
    HashMap label_index;
    label_index.reserve(label_vec.size());

    for (size_t i = 0; i < label_vec.size(); ++i) {
        HashKey key;
        const vector<LocalAssignment> &conditions = label_vec[i].precond;
        for (size_t j = 0; j < conditions.size(); ++j)
            key.push_back(make_pair(conditions[j].local_var, conditions[j].value));
        sort(key.begin(), key.end());
        label_index[key] = i;
    }

    vector<ValueTransitionLabel> old_labels;
    old_labels.swap(label_vec);

    for (HashMap::iterator it = label_index.begin(); it != label_index.end(); ++it) {
        const HashKey &key = it->first;
        int label_no = it->second;
        int powerset_size = (1 << key.size()) - 1; // -1: only consider proper subsets
        bool match = false;
        if (powerset_size <= 31) { // HACK! Don't spend too much time here...
            for (int mask = 0; mask < powerset_size; ++mask) {
                HashKey subset;
                for (size_t i = 0; i < key.size(); ++i)
                    if (mask & (1 << i))
                        subset.push_back(key[i]);
                HashMap::iterator found = label_index.find(subset);
                if (found != label_index.end()) {
                    if (old_labels[label_no].op->get_cost() >= old_labels[found->second].op->get_cost()) {
                        /* TODO: Depending on how clever we want to
                           be, we could prune based on the *adjusted*
                           cost for the respective heuristic instead.
                           This would potentially allow us more pruning
                           when using unit costs as adjusted costs.
                           Seems a minor optimization though.
                        */
                        match = true;
                        break;
                    }
                }
            }
        }
        if (!match)
            label_vec.push_back(old_labels[label_no]);
    }
}
