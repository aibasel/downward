#include "landmark_factory.h"

#include "landmark_graph.h"

#include "exploration.h"
#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../tasks/cost_adapted_task.h"

#include "../utils/memory.h"
#include "../utils/timer.h"

#include <fstream>
#include <limits>

using namespace std;

namespace landmarks {
LandmarkFactory::LandmarkFactory(const options::Options &opts)
    : reasonable_orders(opts.get<bool>("reasonable_orders")),
      only_causal_landmarks(opts.get<bool>("only_causal_landmarks")),
      disjunctive_landmarks(opts.get<bool>("disjunctive_landmarks")),
      conjunctive_landmarks(opts.get<bool>("conjunctive_landmarks")),
      no_orders(opts.get<bool>("no_orders")),
      lm_cost_type(static_cast<OperatorCost>(opts.get_enum("lm_cost_type"))) {
}
/*
  Note: To allow reusing landmark graphs, we use the following temporary
  solution.

  Landmark factories cache the first landmark graph they compute, so
  each call to this function returns the same graph.

  If you want to compute different landmark graphs for different
  Exploration objects, you have to use separate landmark factories.

  This solution remains temporary as long as the question of when and
  how to reuse landmark graphs is open.

  As all heuristics will work on task transformations in the future,
  this function will also get access to a TaskProxy. Then we need to
  ensure that the TaskProxy used by the Exploration object is the same
  as the TaskProxy object passed to this function.
*/
shared_ptr<LandmarkGraph> LandmarkFactory::compute_lm_graph(
    const shared_ptr<AbstractTask> &task, Exploration &exploration) {
    if (lm_graph)
        return lm_graph;
    utils::Timer lm_generation_timer;

    shared_ptr<AbstractTask> cost_adapted_task =
        make_shared<tasks::CostAdaptedTask>(task, lm_cost_type);
    TaskProxy cost_adapted_task_proxy(*cost_adapted_task);

    lm_graph = make_shared<LandmarkGraph>(cost_adapted_task_proxy);
    generate_landmarks(cost_adapted_task, exploration);

    // the following replaces the old "build_lm_graph"
    generate(cost_adapted_task_proxy, exploration);
    cout << "Landmarks generation time: " << lm_generation_timer << endl;
    if (lm_graph->number_of_landmarks() == 0)
        cout << "Warning! No landmarks found. Task unsolvable?" << endl;
    else {
        cout << "Discovered " << lm_graph->number_of_landmarks()
             << " landmarks, of which " << lm_graph->number_of_disj_landmarks()
             << " are disjunctive and "
             << lm_graph->number_of_conj_landmarks() << " are conjunctive \n"
             << lm_graph->number_of_edges() << " edges\n";
    }
    //lm_graph->dump();
    return lm_graph;
}

void LandmarkFactory::generate(const TaskProxy &task_proxy, Exploration &exploration) {
    if (only_causal_landmarks)
        discard_noncausal_landmarks(task_proxy, exploration);
    if (!disjunctive_landmarks)
        discard_disjunctive_landmarks();
    if (!conjunctive_landmarks)
        discard_conjunctive_landmarks();
    lm_graph->set_landmark_ids();

    if (no_orders)
        discard_all_orderings();
    else if (reasonable_orders) {
        cout << "approx. reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, false);
        cout << "approx. obedient reasonable orders" << endl;
        approximate_reasonable_orders(task_proxy, true);
    }
    mk_acyclic_graph();
    lm_graph->set_landmark_cost(calculate_lms_cost());
    calc_achievers(task_proxy, exploration);
}

bool LandmarkFactory::achieves_non_conditional(const OperatorProxy &o,
                                               const LandmarkNode *lmp) const {
    /* Test whether the landmark is achieved by the operator unconditionally.
    A disjunctive landmarks is achieved if one of its disjuncts is achieved. */
    assert(lmp);
    for (EffectProxy effect: o.get_effects()) {
        for (const FactPair &lm_fact : lmp->facts) {
            FactProxy effect_fact = effect.get_fact();
            if (effect_fact.get_pair() == lm_fact) {
                if (effect.get_conditions().empty())
                    return true;
            }
        }
    }
    return false;
}

bool LandmarkFactory::is_landmark_precondition(const OperatorProxy &op,
                                               const LandmarkNode *lmp) const {
    /* Test whether the landmark is used by the operator as a precondition.
    A disjunctive landmarks is used if one of its disjuncts is used. */
    assert(lmp != NULL);
    for (FactProxy pre : op.get_preconditions()) {
        for (const FactPair &lm_fact : lmp->facts) {
            if (pre.get_pair() == lm_fact)
                return true;
        }
    }
    return false;
}

bool LandmarkFactory::relaxed_task_solvable(const TaskProxy &task_proxy,
                                            Exploration &exploration,
                                            vector<vector<int>> &lvl_var,
                                            vector<unordered_map<FactPair, int>> &lvl_op,
                                            bool level_out, const LandmarkNode *exclude, bool compute_lvl_op) const {
    /* Test whether the relaxed planning task is solvable without achieving the propositions in
     "exclude" (do not apply operators that would add a proposition from "exclude").
     As a side effect, collect in lvl_var and lvl_op the earliest possible point in time
     when a proposition / operator can be achieved / become applicable in the relaxed task.
     */

    OperatorsProxy operators = task_proxy.get_operators();
    AxiomsProxy axioms = task_proxy.get_axioms();
    // Initialize lvl_op and lvl_var to numeric_limits<int>::max()
    if (compute_lvl_op) {
        lvl_op.resize(operators.size() + axioms.size());
        for (OperatorProxy op : operators) {
            add_operator_and_propositions_to_list(op, lvl_op);
        }
        for (OperatorProxy axiom : axioms) {
            add_operator_and_propositions_to_list(axiom, lvl_op);
        }
    }
    VariablesProxy variables = task_proxy.get_variables();
    lvl_var.resize(variables.size());
    for (VariableProxy var : variables) {
        lvl_var[var.get_id()].resize(var.get_domain_size(),
                                     numeric_limits<int>::max());
    }
    // Extract propositions from "exclude"
    unordered_set<int> exclude_op_ids;
    vector<FactPair> exclude_props;
    if (exclude) {
        for (OperatorProxy op : operators) {
            if (achieves_non_conditional(op, exclude))
                exclude_op_ids.insert(op.get_id());
        }
        exclude_props.insert(exclude_props.end(),
                             exclude->facts.begin(), exclude->facts.end());
    }
    // Do relaxed exploration
    exploration.compute_reachability_with_excludes(
        lvl_var, lvl_op, level_out, exclude_props, exclude_op_ids, compute_lvl_op);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (FactProxy goal : task_proxy.get_goals())
        if (lvl_var[goal.get_variable().get_id()][goal.get_value()] ==
            numeric_limits<int>::max())
            return false;

    return true;
}

void LandmarkFactory::add_operator_and_propositions_to_list(const OperatorProxy &op,
                                                            vector<unordered_map<FactPair, int>> &lvl_op) const {
    int op_or_axiom_id = get_operator_or_axiom_id(op);
    for (EffectProxy effect : op.get_effects()) {
        lvl_op[op_or_axiom_id].emplace(effect.get_fact().get_pair(), numeric_limits<int>::max());
    }
}

bool LandmarkFactory::is_causal_landmark(const TaskProxy &task_proxy, Exploration &exploration,
                                         const LandmarkNode &landmark) const {
    /* Test whether the relaxed planning task is unsolvable without using any operator
       that has "landmark" has a precondition.
       Similar to "relaxed_task_solvable" above.
     */

    if (landmark.in_goal)
        return true;
    vector<vector<int>> lvl_var;
    vector<unordered_map<FactPair, int>> lvl_op;
    // Initialize lvl_var to numeric_limits<int>::max()
    VariablesProxy variables = task_proxy.get_variables();
    lvl_var.resize(variables.size());
    for (VariableProxy var : variables) {
        lvl_var[var.get_id()].resize(var.get_domain_size(),
                                     numeric_limits<int>::max());
    }
    unordered_set<int> exclude_op_ids;
    vector<FactPair> exclude_props;
    for (OperatorProxy op : task_proxy.get_operators()) {
        if (is_landmark_precondition(op, &landmark)) {
            exclude_op_ids.insert(op.get_id());
        }
    }
    // Do relaxed exploration
    exploration.compute_reachability_with_excludes(
        lvl_var, lvl_op, true, exclude_props, exclude_op_ids, false);

    // Test whether all goal propositions have a level of less than numeric_limits<int>::max()
    for (FactProxy goal : task_proxy.get_goals())
        if (lvl_var[goal.get_variable().get_id()][goal.get_value()] ==
            numeric_limits<int>::max())
            return true;

    return false;
}

bool LandmarkFactory::effect_always_happens(const VariablesProxy &variables,
                                            const EffectsProxy &effects,
                                            set<FactPair> &eff) const {
    /* Test whether the condition of a conditional effect is trivial, i.e. always true.
     We test for the simple case that the same effect proposition is triggered by
     a set of conditions of which one will always be true. This is e.g. the case in
     Schedule, where the effect
     (forall (?oldpaint - colour)
     (when (painted ?x ?oldpaint)
     (not (painted ?x ?oldpaint))))
     is translated by the translator to: if oldpaint == blue, then not painted ?x, and if
     oldpaint == red, then not painted ?x etc.
     If conditional effects are found that are always true, they are returned in "eff".
     */
    // Go through all effects of operator and collect:
    // - all variables that are set to some value in a conditional effect (effect_vars)
    // - variables that can be set to more than one value in a cond. effect (nogood_effect_vars)
    // - a mapping from cond. effect propositions to all the conditions that they appear with
    set<int> effect_vars;
    set<int> nogood_effect_vars;
    map<int, pair<int, vector<FactPair>>> effect_conditions_by_variable;
    for (EffectProxy effect : effects) {
        EffectConditionsProxy effect_conditions = effect.get_conditions();
        FactProxy effect_fact = effect.get_fact();
        int var_id = effect_fact.get_variable().get_id();
        int value = effect_fact.get_value();
        if (effect_conditions.empty() ||
            nogood_effect_vars.find(var_id) != nogood_effect_vars.end()) {
            // Var has no condition or can take on different values, skipping
            continue;
        }
        if (effect_vars.find(var_id) != effect_vars.end()) {
            // We have seen this effect var before
            assert(effect_conditions_by_variable.find(var_id) != effect_conditions_by_variable.end());
            int old_eff = effect_conditions_by_variable.find(var_id)->second.first;
            if (old_eff != value) {
                // Was different effect
                nogood_effect_vars.insert(var_id);
                continue;
            }
        } else {
            // We have not seen this effect var before
            effect_vars.insert(var_id);
        }
        if (effect_conditions_by_variable.find(var_id) != effect_conditions_by_variable.end()
            && effect_conditions_by_variable.find(var_id)->second.first == value) {
            // We have seen this effect before, adding conditions
            for (FactProxy effect_condition : effect_conditions) {
                vector<FactPair> &vec = effect_conditions_by_variable.find(var_id)->second.second;
                vec.push_back(effect_condition.get_pair());
            }
        } else {
            // We have not seen this effect before, making new effect entry
            vector<FactPair> &vec = effect_conditions_by_variable.emplace(
                var_id, make_pair(
                    value, vector<FactPair> ())).first->second.second;
            for (FactProxy effect_condition : effect_conditions) {
                vec.push_back(effect_condition.get_pair());
            }
        }
    }

    // For all those effect propositions whose variables do not take on different values...
    for (const auto &effect_conditions : effect_conditions_by_variable) {
        if (nogood_effect_vars.find(effect_conditions.first) != nogood_effect_vars.end()) {
            continue;
        }
        // ...go through all the conditions that the effect has, and map condition
        // variables to the set of values they take on (in unique_conds)
        map<int, set<int>> unique_conds;
        for (const FactPair &cond : effect_conditions.second.second) {
            if (unique_conds.find(cond.var) != unique_conds.end()) {
                unique_conds.find(cond.var)->second.insert(
                    cond.value);
            } else {
                set<int> &the_set = unique_conds.emplace(cond.var, set<int>()).first->second;
                the_set.insert(cond.value);
            }
        }
        // Check for each condition variable whether the number of values it takes on is
        // equal to the domain of that variable...
        bool is_always_reached = true;
        for (auto &unique_cond : unique_conds) {
            bool is_surely_reached_by_var = false;
            int num_values_for_cond = unique_cond.second.size();
            int num_values_of_variable = variables[unique_cond.first].get_domain_size();
            if (num_values_for_cond == num_values_of_variable) {
                is_surely_reached_by_var = true;
            }
            // ...or else if the condition variable is the same as the effect variable,
            // check whether the condition variable takes on all other values except the
            // effect value
            else if (unique_cond.first == effect_conditions.first &&
                     num_values_for_cond == num_values_of_variable - 1) {
                // Number of different values is correct, now ensure that the effect value
                // was the one missing
                unique_cond.second.insert(effect_conditions.second.first);
                num_values_for_cond = unique_cond.second.size();
                if (num_values_for_cond == num_values_of_variable) {
                    is_surely_reached_by_var = true;
                }
            }
            // If one of the condition variables does not fulfill the criteria, the effect
            // is not certain to happen
            if (!is_surely_reached_by_var)
                is_always_reached = false;
        }
        if (is_always_reached)
            eff.insert(FactPair(
                           effect_conditions.first, effect_conditions.second.first));
    }
    return eff.empty();
}

bool LandmarkFactory::interferes(const TaskProxy &task_proxy,
                                 const LandmarkNode *node_a,
                                 const LandmarkNode *node_b) const {
    /* Facts a and b interfere (i.e., achieving b before a would mean having to delete b
     and re-achieve it in order to achieve a) if one of the following condition holds:
     1. a and b are mutex
     2. All actions that add a also add e, and e and b are mutex
     3. There is a greedy necessary predecessor x of a, and x and b are mutex
     This is the definition of Hoffmann et al. except that they have one more condition:
     "all actions that add a delete b". However, in our case (SAS+ formalism), this condition
     is the same as 2.
     */
    assert(node_a != node_b);
    assert(!node_a->disjunctive && !node_b->disjunctive);

    VariablesProxy variables = task_proxy.get_variables();
    for (const FactPair &lm_fact_b : node_b->facts) {
        FactProxy fact_b = variables[lm_fact_b.var].get_fact(lm_fact_b.value);
        for (const FactPair &lm_fact_a : node_a->facts) {
            FactProxy fact_a = variables[lm_fact_a.var].get_fact(lm_fact_a.value);
            if (lm_fact_a == lm_fact_b) {
                if (!node_a->conjunctive || !node_b->conjunctive)
                    return false;
                else
                    continue;
            }

            // 1. a, b mutex
            if (fact_a.is_mutex(fact_b))
                return true;

            // 2. Shared effect e in all operators reaching a, and e, b are mutex
            // Skip this for conjunctive nodes a, as they are typically achieved through a
            // sequence of operators successively adding the parts of a
            if (node_a->conjunctive)
                continue;

            unordered_map<int, int> shared_eff;
            bool init = true;
            const vector<int> &op_or_axiom_ids = lm_graph->get_operators_including_eff(lm_fact_a);
            // Intersect operators that achieve a one by one
            for (int op_or_axiom_id : op_or_axiom_ids) {
                // If no shared effect among previous operators, break
                if (!init && shared_eff.empty())
                    break;
                // Else, insert effects of this operator into set "next_eff" if
                // it is an unconditional effect or a conditional effect that is sure to
                // happen. (Such "trivial" conditions can arise due to our translator,
                // e.g. in Schedule. There, the same effect is conditioned on a disjunction
                // of conditions of which one will always be true. We test for a simple kind
                // of these trivial conditions here.)
                EffectsProxy effects = get_operator_or_axiom(task_proxy, op_or_axiom_id).get_effects();
                set<FactPair> trivially_conditioned_effects;
                bool trivial_conditioned_effects_found = effect_always_happens(variables, effects,
                                                                               trivially_conditioned_effects);
                unordered_map<int, int> next_eff;
                for (EffectProxy effect : effects) {
                    FactPair effect_fact = effect.get_fact().get_pair();
                    if (effect.get_conditions().empty() &&
                        effect_fact.var != lm_fact_a.var) {
                        next_eff.emplace(effect_fact.var, effect_fact.value);
                    } else if (trivial_conditioned_effects_found &&
                               trivially_conditioned_effects.find(effect_fact)
                               != trivially_conditioned_effects.end())
                        next_eff.emplace(effect_fact.var, effect_fact.value);
                }
                // Intersect effects of this operator with those of previous operators
                if (init)
                    swap(shared_eff, next_eff);
                else {
                    unordered_map<int, int> result;
                    for (const auto &eff1 : shared_eff) {
                        auto it2 = next_eff.find(eff1.first);
                        if (it2 != next_eff.end() && it2->second == eff1.second)
                            result.insert(eff1);
                    }
                    swap(shared_eff, result);
                }
                init = false;
            }
            // Test whether one of the shared effects is inconsistent with b
            for (const pair<int, int> &eff : shared_eff) {
                const FactProxy &effect_fact = variables[eff.first].get_fact(eff.second);
                if (effect_fact != fact_a &&
                    effect_fact != fact_b &&
                    effect_fact.is_mutex(fact_b))
                    return true;
            }
        }

        /* // Experimentally commenting this out -- see issue202.
        // 3. Exists LM x, inconsistent x, b and x->_gn a
        for (const auto &parent : node_a->parents) {
            const LandmarkNode &node = *parent.first;
            edge_type edge = parent.second;
            for (const FactPair &parent_prop : node.facts) {
                const FactProxy &parent_prop_fact =
                    variables[parent_prop.var].get_fact(parent_prop.value);
                if (edge >= greedy_necessary &&
                    parent_prop_fact != fact_b &&
                    parent_prop_fact.is_mutex(fact_b))
                    return true;
            }
        }
        */
    }
    // No inconsistency found
    return false;
}

void LandmarkFactory::approximate_reasonable_orders(
    const TaskProxy &task_proxy, bool obedient_orders) {
    /*
      Approximate reasonable and obedient reasonable orders according
      to Hoffmann et al. If flag "obedient_orders" is true, we
      calculate obedient reasonable orders, otherwise reasonable
      orders.

      If node_p is in goal, then any node2_p which interferes with
      node_p can be reasonably ordered before node_p. Otherwise, if
      node_p is greedy necessary predecessor of node2, and there is
      another predecessor "parent" of node2, then parent and all
      predecessors of parent can be ordered reasonably before node_p if
      they interfere with node_p.
    */
    State initial_state = task_proxy.get_initial_state();
    int variables_size = task_proxy.get_variables().size();
    for (LandmarkNode *node_p : lm_graph->get_nodes()) {
        if (node_p->disjunctive)
            continue;

        if (node_p->is_true_in_state(initial_state))
            return;

        if (!obedient_orders && node_p->is_goal()) {
            for (LandmarkNode *node2_p : lm_graph->get_nodes()) {
                if (node2_p == node_p || node2_p->disjunctive)
                    continue;
                if (interferes(task_proxy, node2_p, node_p)) {
                    edge_add(*node2_p, *node_p, EdgeType::reasonable);
                }
            }
        } else {
            // Collect candidates for reasonable orders in "interesting nodes".
            // Use hash set to filter duplicates.
            unordered_set<LandmarkNode *> interesting_nodes(variables_size);
            for (const auto &child : node_p->children) {
                const LandmarkNode &node2 = *child.first;
                const EdgeType &edge2 = child.second;
                if (edge2 >= EdgeType::greedy_necessary) { // found node2: node_p ->_gn node2
                    for (const auto &p : node2.parents) {   // find parent
                        LandmarkNode &parent = *(p.first);
                        const EdgeType &edge = p.second;
                        if (parent.disjunctive)
                            continue;
                        if ((edge >= EdgeType::natural || (obedient_orders && edge == EdgeType::reasonable)) &&
                            &parent != node_p) {  // find predecessors or parent and collect in
                            // "interesting nodes"
                            interesting_nodes.insert(&parent);
                            collect_ancestors(interesting_nodes, parent,
                                              obedient_orders);
                        }
                    }
                }
            }
            // Insert reasonable orders between those members of "interesting nodes" that interfere
            // with node_p.
            for (LandmarkNode *node : interesting_nodes) {
                if (node == node_p || node->disjunctive)
                    continue;
                if (interferes(task_proxy, node, node_p)) {
                    if (!obedient_orders)
                        edge_add(*node, *node_p, EdgeType::reasonable);
                    else
                        edge_add(*node, *node_p, EdgeType::obedient_reasonable);
                }
            }
        }
    }
}

void LandmarkFactory::collect_ancestors(
    unordered_set<LandmarkNode *> &result,
    LandmarkNode &node,
    bool use_reasonable) {
    /* Returns all ancestors in the landmark graph of landmark node "start" */

    // There could be cycles if use_reasonable == true
    list<LandmarkNode *> open_nodes;
    unordered_set<LandmarkNode *> closed_nodes;
    for (const auto &p : node.parents) {
        LandmarkNode &parent = *(p.first);
        const EdgeType &edge = p.second;
        if (edge >= EdgeType::natural || (use_reasonable && edge == EdgeType::reasonable))
            if (closed_nodes.count(&parent) == 0) {
                open_nodes.push_back(&parent);
                closed_nodes.insert(&parent);
                result.insert(&parent);
            }

    }
    while (!open_nodes.empty()) {
        LandmarkNode &node2 = *(open_nodes.front());
        for (const auto &p : node2.parents) {
            LandmarkNode &parent = *(p.first);
            const EdgeType &edge = p.second;
            if (edge >= EdgeType::natural || (use_reasonable && edge == EdgeType::reasonable)) {
                if (closed_nodes.count(&parent) == 0) {
                    open_nodes.push_back(&parent);
                    closed_nodes.insert(&parent);
                    result.insert(&parent);
                }
            }
        }
        open_nodes.pop_front();
    }
}

void LandmarkFactory::edge_add(LandmarkNode &from, LandmarkNode &to,
                               EdgeType type) {
    /* Adds an edge in the landmarks graph if there is no contradicting edge (simple measure to
    reduce cycles. If the edge is already present, the stronger edge type wins.
    */
    assert(&from != &to);
    assert(from.parents.find(&to) == from.parents.end() || type <= EdgeType::reasonable);
    assert(to.children.find(&from) == to.children.end() || type <= EdgeType::reasonable);

    if (type == EdgeType::reasonable || type == EdgeType::obedient_reasonable) { // simple cycle test
        if (from.parents.find(&to) != from.parents.end()) { // Edge in opposite direction exists
            //cout << "edge in opposite direction exists" << endl;
            if (from.parents.find(&to)->second > type) // Stronger order present, return
                return;
            // Edge in opposite direction is weaker, delete
            from.parents.erase(&to);
            to.children.erase(&from);
        }
    }

    // If edge already exists, remove if weaker
    if (from.children.find(&to) != from.children.end() && from.children.find(
            &to)->second < type) {
        from.children.erase(&to);
        assert(to.parents.find(&from) != to.parents.end());
        to.parents.erase(&from);

        assert(to.parents.find(&from) == to.parents.end());
        assert(from.children.find(&to) == from.children.end());
    }
    // If edge does not exist (or has just been removed), insert
    if (from.children.find(&to) == from.children.end()) {
        assert(to.parents.find(&from) == to.parents.end());
        from.children.emplace(&to, type);
        to.parents.emplace(&from, type);
        //cout << "added parent with address " << &from << endl;
    }
    assert(from.children.find(&to) != from.children.end());
    assert(to.parents.find(&from) != to.parents.end());
}

void LandmarkFactory::discard_noncausal_landmarks(const TaskProxy &task_proxy, Exploration &exploration) {
    int number_of_noncausal_landmarks = 0;
    bool change = true;
    VariablesProxy variables = task_proxy.get_variables();
    while (change) {
        change = false;
        for (LandmarkNode *landmark_node : lm_graph->get_nodes()) {
            if (!is_causal_landmark(task_proxy, exploration, *landmark_node)) {
                cout << "Discarding non-causal landmark: ";
                lm_graph->dump_node(variables, landmark_node);
                lm_graph->rm_landmark_node(landmark_node);
                ++number_of_noncausal_landmarks;
                change = true;
                break;
            }
        }
    }
    cout << "Discarded " << number_of_noncausal_landmarks
         << " non-causal landmarks" << endl;
}

void LandmarkFactory::discard_disjunctive_landmarks() {
    /* Using disjunctive landmarks during landmark generation can be
    beneficial even if we don't want to use disunctive landmarks during s
    search. This function deletes all disjunctive landmarks that have been
    found. (Note: this is implemented inefficiently because "nodes" contains
    pointers, not the actual nodes. We should change that.)
    */
    if (lm_graph->number_of_disj_landmarks() == 0)
        return;
    cout << "Discarding " << lm_graph->number_of_disj_landmarks()
         << " disjunctive landmarks" << endl;
    bool change = true;
    while (change) {
        change = false;
        for (LandmarkNode *node : lm_graph->get_nodes()) {
            if (node->disjunctive) {
                lm_graph->rm_landmark_node(node);
                change = true;
                break;
            }
        }
    }
    // [Malte] Commented out the following assertions because
    // the old methods for this are no longer available.
    // assert(lm_graph->number_of_disj_landmarks() == 0);
    // assert(disj_lms_to_nodes.size() == 0);
}

void LandmarkFactory::discard_conjunctive_landmarks() {
    if (lm_graph->number_of_conj_landmarks() == 0)
        return;
    cout << "Discarding " << lm_graph->number_of_conj_landmarks()
         << " conjunctive landmarks" << endl;
    bool change = true;
    while (change) {
        change = false;
        for (LandmarkNode *node : lm_graph->get_nodes()) {
            if (node->conjunctive) {
                lm_graph->rm_landmark_node(node);
                change = true;
                break;
            }
        }
    }
    // [Malte] Commented out the following assertion because
    // the old method for this is no longer available.
    // assert(number_of_conj_landmarks() == 0);
}

void LandmarkFactory::discard_all_orderings() {
    cout << "Removing all orderings." << endl;
    for (LandmarkNode *node : lm_graph->get_nodes()) {
        node->children.clear();
        node->parents.clear();
    }
}

void LandmarkFactory::mk_acyclic_graph() {
    unordered_set<LandmarkNode *> acyclic_node_set(lm_graph->number_of_landmarks());
    int removed_edges = 0;
    for (LandmarkNode *node : lm_graph->get_nodes()) {
        if (acyclic_node_set.find(node) == acyclic_node_set.end())
            removed_edges += loop_acyclic_graph(*node, acyclic_node_set);
    }
    // [Malte] Commented out the following assertion because
    // the old method for this is no longer available.
    // assert(acyclic_node_set.size() == number_of_landmarks());
    cout << "Removed " << removed_edges
         << " reasonable or obedient reasonable orders\n";
}

bool LandmarkFactory::remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                                      list<pair<LandmarkNode *, EdgeType>> &path,
                                                      list<pair<LandmarkNode *, EdgeType>>::iterator it) {
    LandmarkNode *parent_p = 0;
    LandmarkNode *child_p = 0;
    for (list<pair<LandmarkNode *, EdgeType>>::iterator it2 = it; it2
         != path.end(); ++it2) {
        EdgeType edge = it2->second;
        if (edge == EdgeType::reasonable || edge == EdgeType::obedient_reasonable) {
            parent_p = it2->first;
            if (*it2 == path.back()) {
                child_p = cur;
                break;
            } else {
                list<pair<LandmarkNode *, EdgeType>>::iterator child_it = it2;
                ++child_it;
                child_p = child_it->first;
            }
            if (edge == EdgeType::obedient_reasonable)
                break;
            // else no break since o_r order could still appear in list
        }
    }
    assert(parent_p != 0 && child_p != 0);
    assert(parent_p->children.find(child_p) != parent_p->children.end());
    assert(child_p->parents.find(parent_p) != child_p->parents.end());
    parent_p->children.erase(child_p);
    child_p->parents.erase(parent_p);
    return true;
}

int LandmarkFactory::loop_acyclic_graph(LandmarkNode &lmn,
                                        unordered_set<LandmarkNode *> &acyclic_node_set) {
    assert(acyclic_node_set.find(&lmn) == acyclic_node_set.end());
    int nr_removed = 0;
    list<pair<LandmarkNode *, EdgeType>> path;
    unordered_set<LandmarkNode *> visited = unordered_set<LandmarkNode *>(lm_graph->number_of_landmarks());
    LandmarkNode *cur = &lmn;
    while (true) {
        assert(acyclic_node_set.find(cur) == acyclic_node_set.end());
        if (visited.find(cur) != visited.end()) { // cycle
            // find other occurrence of cur node in path
            list<pair<LandmarkNode *, EdgeType>>::iterator it;
            for (it = path.begin(); it != path.end(); ++it) {
                if (it->first == cur)
                    break;
            }
            assert(it != path.end());
            // remove edge from graph
            remove_first_weakest_cycle_edge(cur, path, it);
            //assert(removed);
            ++nr_removed;

            path.clear();
            cur = &lmn;
            visited.clear();
            continue;
        }
        visited.insert(cur);
        bool empty = true;
        for (const auto &child : cur->children) {
            LandmarkNode *child_p = child.first;
            EdgeType edge = child.second;
            if (acyclic_node_set.find(child_p) == acyclic_node_set.end()) {
                path.emplace_back(cur, edge);
                cur = child_p;
                empty = false;
                break;
            }
        }
        if (!empty)
            continue;

        // backtrack
        visited.erase(cur);
        acyclic_node_set.insert(cur);
        if (!path.empty()) {
            cur = path.back().first;
            path.pop_back();
            visited.erase(cur);
        } else
            break;
    }
    assert(acyclic_node_set.find(&lmn) != acyclic_node_set.end());
    return nr_removed;
}

int LandmarkFactory::calculate_lms_cost() const {
    int result = 0;
    for (LandmarkNode *lmn : lm_graph->get_nodes())
        result += lmn->min_cost;

    return result;
}

void LandmarkFactory::compute_predecessor_information(
    const TaskProxy &task_proxy,
    Exploration &exploration,
    LandmarkNode *bp,
    vector<vector<int>> &lvl_var,
    vector<unordered_map<FactPair, int>> &lvl_op) {
    /* Collect information at what time step propositions can be reached
    (in lvl_var) in a relaxed plan that excludes bp, and similarly
    when operators can be applied (in lvl_op).  */

    relaxed_task_solvable(task_proxy, exploration, lvl_var, lvl_op, true, bp);
}

void LandmarkFactory::calc_achievers(const TaskProxy &task_proxy, Exploration &exploration) {
    VariablesProxy variables = task_proxy.get_variables();
    for (LandmarkNode *lmn : lm_graph->get_nodes()) {
        for (const FactPair &lm_fact : lmn->facts) {
            const vector<int> &ops = lm_graph->get_operators_including_eff(lm_fact);
            lmn->possible_achievers.insert(ops.begin(), ops.end());

            if (variables[lm_fact.var].is_derived())
                lmn->is_derived = true;
        }

        vector<vector<int>> lvl_var;
        vector<unordered_map<FactPair, int>> lvl_op;
        compute_predecessor_information(task_proxy, exploration, lmn, lvl_var, lvl_op);

        for (int op_or_axom_id : lmn->possible_achievers) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axom_id);

            if (_possibly_reaches_lm(op, lvl_var, lmn)) {
                lmn->first_achievers.insert(op_or_axom_id);
            }
        }
    }
}

void _add_options_to_parser(OptionParser &parser) {
    parser.add_option<bool>("reasonable_orders",
                            "generate reasonable orders",
                            "false");
    parser.add_option<bool>("only_causal_landmarks",
                            "keep only causal landmarks",
                            "false");
    parser.add_option<bool>("disjunctive_landmarks",
                            "keep disjunctive landmarks",
                            "true");
    parser.add_option<bool>("conjunctive_landmarks",
                            "keep conjunctive landmarks",
                            "true");
    parser.add_option<bool>("no_orders",
                            "discard all orderings",
                            "false");

    /* TODO: This option should go away anyway once the landmark code
       is properly cleaned up. */
    vector<string> cost_types;
    cost_types.push_back("NORMAL");
    cost_types.push_back("ONE");
    cost_types.push_back("PLUSONE");
    parser.add_enum_option("lm_cost_type",
                           cost_types,
                           "landmark action cost adjustment",
                           "NORMAL");
}


static PluginTypePlugin<LandmarkFactory> _type_plugin(
    "LandmarkFactory",
    "A landmark factory specification is either a newly created "
    "instance or a landmark factory that has been defined previously. "
    "This page describes how one can specify a new landmark factory instance. "
    "For re-using landmark factories, see OptionSyntax#Landmark_Predefinitions.\n\n"
    "**Warning:** See OptionCaveats for using cost types with Landmarks");
}
