#include "domain_transition_graph.h"

#include "../utils/hash.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

namespace domain_transition_graph {
DTGFactory::DTGFactory(const TaskProxy &task_proxy,
                       bool collect_transition_side_effects,
                       const function<bool(int, int)> &pruning_condition)
    : task_proxy(task_proxy),
      collect_transition_side_effects(collect_transition_side_effects),
      pruning_condition(pruning_condition) {
}

DTGFactory::DTGs DTGFactory::build_dtgs() {
    DTGs dtgs;

    allocate_graphs_and_nodes(dtgs);
    initialize_index_structures(dtgs.size());
    create_transitions(dtgs);
    simplify_transitions(dtgs);
    if (collect_transition_side_effects)
        collect_all_side_effects(dtgs);
    return dtgs;
}

void DTGFactory::allocate_graphs_and_nodes(DTGs &dtgs) {
    VariablesProxy variables = task_proxy.get_variables();
    dtgs.resize(variables.size());
    for (VariableProxy var : variables) {
        int var_id = var.get_id();
        int range = var.get_domain_size();
        dtgs[var_id] = utils::make_unique_ptr<DomainTransitionGraph>(var_id, range);
    }
}

void DTGFactory::initialize_index_structures(int num_dtgs) {
    transition_index.clear();
    transition_index.resize(num_dtgs);
    global_to_local_var.clear();
    global_to_local_var.resize(num_dtgs);
}

void DTGFactory::create_transitions(DTGs &dtgs) {
    for (OperatorProxy op : task_proxy.get_operators())
        for (EffectProxy eff : op.get_effects())
            process_effect(eff, op, dtgs);
    for (OperatorProxy ax : task_proxy.get_axioms())
        for (EffectProxy eff : ax.get_effects())
            process_effect(eff, ax, dtgs);
}

void DTGFactory::process_effect(const EffectProxy &eff, const OperatorProxy &op,
                                DTGs &dtgs) {
    FactProxy fact = eff.get_fact();
    int var_id = fact.get_variable().get_id();
    DomainTransitionGraph *dtg = dtgs[var_id].get();
    int origin = -1;
    int target = fact.get_value();
    vector<LocalAssignment> transition_condition;
    vector<LocalAssignment> side_effect;
    unsigned int first_new_local_var = dtg->local_to_global_child.size();
    for (FactProxy pre : op.get_preconditions()) {
        if (pre.get_variable() == fact.get_variable())
            origin = pre.get_value();
        else
            update_transition_condition(pre, dtg, transition_condition);
    }
    for (FactProxy cond : eff.get_conditions()) {
        if (cond.get_variable() == fact.get_variable()) {
            if (origin != -1 && cond.get_value() != origin) {
                revert_new_local_vars(dtg, first_new_local_var);
                return; // conflicting condition on effect variable
            }
            origin = cond.get_value();
        } else {
            update_transition_condition(cond, dtg, transition_condition);
        }
    }
    if (target == origin) {
        revert_new_local_vars(dtg, first_new_local_var);
        return;
    }
    if (origin != -1) {
        ValueTransition *trans = get_transition(origin, target, dtg);
        trans->labels.push_back(
            ValueTransitionLabel(op.get_id(), op.is_axiom(), transition_condition, side_effect));
    } else {
        int domain_size = fact.get_variable().get_domain_size();
        for (int origin = 0; origin < domain_size; ++origin) {
            if (origin == target)
                continue;
            ValueTransition *trans = get_transition(origin, target, dtg);
            trans->labels.push_back(
                ValueTransitionLabel(op.get_id(), op.is_axiom(), transition_condition, side_effect));
        }
    }
}

void DTGFactory::update_transition_condition(const FactProxy &fact,
                                             DomainTransitionGraph *dtg,
                                             vector<LocalAssignment> &condition) {
    int fact_var = fact.get_variable().get_id();
    if (!pruning_condition(dtg->var, fact_var)) {
        extend_global_to_local_mapping_if_necessary(dtg, fact_var);
        int local_var = global_to_local_var[dtg->var][fact_var];
        condition.push_back(LocalAssignment(local_var, fact.get_value()));
    }
}

void DTGFactory::extend_global_to_local_mapping_if_necessary(
    DomainTransitionGraph *dtg, int global_var) {
    if (!global_to_local_var[dtg->var].count(global_var)) {
        global_to_local_var[dtg->var][global_var] = dtg->local_to_global_child.size();
        dtg->local_to_global_child.push_back(global_var);
    }
}

void DTGFactory::revert_new_local_vars(DomainTransitionGraph *dtg,
                                       unsigned int first_local_var) {
    vector<int> &loc_to_glob = dtg->local_to_global_child;
    for (unsigned int l = first_local_var; l < loc_to_glob.size(); ++l)
        global_to_local_var[dtg->var].erase(loc_to_glob[l]);
    if (loc_to_glob.size() > first_local_var)
        loc_to_glob.erase(loc_to_glob.begin() + first_local_var,
                          loc_to_glob.end());
}

ValueTransition *DTGFactory::get_transition(int origin, int target,
                                            DomainTransitionGraph *dtg) {
    utils::HashMap<pair<int, int>, int> &trans_map = transition_index[dtg->var];
    pair<int, int> arc = make_pair(origin, target);
    ValueNode &origin_node = dtg->nodes[origin];
    // create new transition if necessary
    if (!trans_map.count(arc)) {
        trans_map[arc] = origin_node.transitions.size();
        ValueNode &target_node = dtg->nodes[target];
        origin_node.transitions.push_back(ValueTransition(&target_node));
    }
    return &origin_node.transitions[trans_map[arc]];
}

void DTGFactory::collect_all_side_effects(DTGs &dtgs) {
    for (auto &dtg : dtgs) {
        for (auto &node : dtg->nodes)
            for (auto &transition: node.transitions)
                collect_side_effects(dtg.get(), transition.labels);
    }
}

void DTGFactory::collect_side_effects(DomainTransitionGraph *dtg,
                                      vector<ValueTransitionLabel> &labels) {
    const vector<int> &loc_to_glob = dtg->local_to_global_child;
    const unordered_map<int, int> &glob_to_loc = global_to_local_var[dtg->var];

    for (auto &label : labels) {
        // create global condition for label
        vector<FactPair> precond_pairs;
        for (auto &assignment : label.precond) {
            int var = loc_to_glob[assignment.local_var];
            precond_pairs.emplace_back(var, assignment.value);
        }
        sort(precond_pairs.begin(), precond_pairs.end());

        // collect operator precondition
        OperatorProxy op = get_op_for_label(label);
        unordered_map<int, int> pre_map;
        for (FactProxy pre : op.get_preconditions())
            pre_map[pre.get_variable().get_id()] = pre.get_value();

        // collect side effect from each operator effect
        vector<LocalAssignment> side_effects;
        for (EffectProxy eff : op.get_effects()) {
            int var_no = eff.get_fact().get_variable().get_id();
            if (var_no == dtg->var || !glob_to_loc.count(var_no)) {
                // This is either an effect on the variable we're
                // building the DTG for, or an effect on a variable we
                // don't need to track because it doesn't appear in
                // conditions of this DTG. Ignore it.
                continue;
            }

            int pre = -1;
            auto pre_it = pre_map.find(var_no);
            if (pre_it != pre_map.end())
                pre = pre_it->second;
            int post = eff.get_fact().get_value();

            vector<FactPair> triggercond_pairs;
            if (pre != -1)
                triggercond_pairs.emplace_back(var_no, pre);

            for (FactProxy condition : eff.get_conditions()) {
                int c_var_id = condition.get_variable().get_id();
                int c_val = condition.get_value();
                triggercond_pairs.emplace_back(c_var_id, c_val);
            }
            sort(triggercond_pairs.begin(), triggercond_pairs.end());

            if (includes(precond_pairs.begin(), precond_pairs.end(),
                         triggercond_pairs.begin(), triggercond_pairs.end())) {
                int local_var = glob_to_loc.at(var_no);
                side_effects.push_back(LocalAssignment(local_var, post));
            }
        }
        label.effect = side_effects;
    }
}

void DTGFactory::simplify_transitions(DTGs &dtgs) {
    for (auto &dtg : dtgs)
        for (ValueNode & node : dtg->nodes)
            for (ValueTransition & transition : node.transitions)
                simplify_labels(transition.labels);
}

OperatorProxy DTGFactory::get_op_for_label(const ValueTransitionLabel &label) {
    if (label.is_axiom)
        return task_proxy.get_axioms()[label.op_id];
    return task_proxy.get_operators()[label.op_id];
}

void DTGFactory::simplify_labels(vector<ValueTransitionLabel> &labels) {
    // Remove labels with duplicate or dominated conditions.

    /*
      Algorithm: Put all transitions into an unordered_map
      (key: condition, value: index in label list).
      This already gets rid of transitions with identical conditions.

      Then go through the unordered_map, checking for each element if
      none of the subset conditions are part of the unordered_map.
      Put the element into the new labels list iff this is the case.
     */

    using HashKey = vector<FactPair>;
    using HashMap = utils::HashMap<HashKey, int>;
    HashMap label_index;
    label_index.reserve(labels.size());

    for (size_t i = 0; i < labels.size(); ++i) {
        HashKey key;
        for (LocalAssignment &assign : labels[i].precond)
            key.emplace_back(assign.local_var, assign.value);
        sort(key.begin(), key.end());
        label_index[key] = i;
    }

    vector<ValueTransitionLabel> old_labels;
    old_labels.swap(labels);

    for (auto &entry : label_index) {
        const HashKey &key = entry.first;
        int label_no = entry.second;
        int powerset_size = (1 << key.size()) - 1; // -1: only consider proper subsets
        bool match = false;
        if (powerset_size <= 31) { // HACK! Don't spend too much time here...
            OperatorProxy op = get_op_for_label(old_labels[label_no]);
            for (int mask = 0; mask < powerset_size; ++mask) {
                HashKey subset;
                for (size_t i = 0; i < key.size(); ++i)
                    if (mask & (1 << i))
                        subset.push_back(key[i]);
                HashMap::iterator found = label_index.find(subset);
                if (found != label_index.end()) {
                    const ValueTransitionLabel &f_label = old_labels[found->second];
                    OperatorProxy f_op = get_op_for_label(f_label);
                    if (op.get_cost() >= f_op.get_cost()) {
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
            labels.push_back(old_labels[label_no]);
    }
}


DomainTransitionGraph::DomainTransitionGraph(int var_index, int node_count) {
    var = var_index;
    nodes.reserve(node_count);
    for (int value = 0; value < node_count; ++value)
        nodes.push_back(ValueNode(this, value));
    last_helpful_transition_extraction_time = -1;
}
}
