#include "landmark_factory.h"

#include "landmark_graph.h"

#include "util.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"
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
      lm_graph_task(nullptr) {
}
/*
  TODO: Update this comment

  Note: To allow reusing landmark graphs, we use the following temporary
  solution.

  Landmark factories cache the first landmark graph they compute, so
  each call to this function returns the same graph. Asking for landmark graphs
  of different tasks is an error and will exit with SEARCH_UNSUPPORTED.

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
    const shared_ptr<AbstractTask> &task) {
    if (lm_graph) {
        if (lm_graph_task != task.get()) {
            cerr << "LandmarkFactory was asked to compute landmark graphs for "
                 << "two different tasks. This is currently not supported."
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
        }
        return lm_graph;
    }
    lm_graph_task = task.get();
    utils::Timer lm_generation_timer;

    TaskProxy task_proxy(*task);
    lm_graph = make_shared<LandmarkGraph>(task_proxy);
    generate_landmarks(task);

    utils::g_log << "Landmarks generation time: " << lm_generation_timer << endl;
    if (lm_graph->number_of_landmarks() == 0)
        utils::g_log << "Warning! No landmarks found. Task unsolvable?" << endl;
    else {
        utils::g_log << "Discovered " << lm_graph->number_of_landmarks()
                     << " landmarks, of which " << lm_graph->number_of_disj_landmarks()
                     << " are disjunctive and "
                     << lm_graph->number_of_conj_landmarks() << " are conjunctive." << endl;
        utils::g_log << lm_graph->number_of_edges() << " edges" << endl;
    }
    //lm_graph->dump();
    return lm_graph;
}

bool LandmarkFactory::is_landmark_precondition(const OperatorProxy &op,
                                               const LandmarkNode *lmp) const {
    /* Test whether the landmark is used by the operator as a precondition.
    A disjunctive landmarks is used if one of its disjuncts is used. */
    assert(lmp);
    for (FactProxy pre : op.get_preconditions()) {
        for (const FactPair &lm_fact : lmp->facts) {
            if (pre.get_pair() == lm_fact)
                return true;
        }
    }
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
            for (const pair<const int, int> &eff : shared_eff) {
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
    for (auto &node_p : lm_graph->get_nodes()) {
        if (node_p->disjunctive)
            continue;

        if (node_p->is_true_in_state(initial_state))
            return;

        if (!obedient_orders && node_p->is_goal()) {
            for (auto &node2_p : lm_graph->get_nodes()) {
                if (node2_p == node_p || node2_p->disjunctive)
                    continue;
                if (interferes(task_proxy, node2_p.get(), node_p.get())) {
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
                            &parent != node_p.get()) {  // find predecessors or parent and collect in
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
                if (node == node_p.get() || node->disjunctive)
                    continue;
                if (interferes(task_proxy, node, node_p.get())) {
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
            //utils::g_log << "edge in opposite direction exists" << endl;
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
        //utils::g_log << "added parent with address " << &from << endl;
    }
    assert(from.children.find(&to) != from.children.end());
    assert(to.parents.find(&from) != to.parents.end());
}

void LandmarkFactory::discard_disjunctive_landmarks() {
    /*
      Using disjunctive landmarks during landmark generation can be beneficial
      even if we don't want to use disjunctive landmarks during search. So we
      allow removing disjunctive landmarks after landmark generation.
    */
    if (lm_graph->number_of_disj_landmarks() > 0) {
        utils::g_log << "Discarding " << lm_graph->number_of_disj_landmarks()
                     << " disjunctive landmarks" << endl;
        lm_graph->remove_node_if(
            [](const LandmarkNode &node) {return node.disjunctive;});
    }
}

void LandmarkFactory::discard_conjunctive_landmarks() {
    if (lm_graph->number_of_conj_landmarks() > 0) {
        utils::g_log << "Discarding " << lm_graph->number_of_conj_landmarks()
                     << " conjunctive landmarks" << endl;
        lm_graph->remove_node_if(
            [](const LandmarkNode &node) {return node.conjunctive;});
    }
}

void LandmarkFactory::discard_all_orderings() {
    utils::g_log << "Removing all orderings." << endl;
    for (auto &node : lm_graph->get_nodes()) {
        node->children.clear();
        node->parents.clear();
    }
}

int LandmarkFactory::calculate_lms_cost() const {
    int result = 0;
    for (auto &lmn : lm_graph->get_nodes())
        result += lmn->min_cost;

    return result;
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
}


static PluginTypePlugin<LandmarkFactory> _type_plugin(
    "LandmarkFactory",
    "A landmark factory specification is either a newly created "
    "instance or a landmark factory that has been defined previously. "
    "This page describes how one can specify a new landmark factory instance. "
    "For re-using landmark factories, see OptionSyntax#Landmark_Predefinitions.",
    "landmarks");
}
