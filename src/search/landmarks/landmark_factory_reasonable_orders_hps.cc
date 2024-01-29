#include "landmark_factory_reasonable_orders_hps.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "util.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"

using namespace std;
namespace landmarks {
LandmarkFactoryReasonableOrdersHPS::LandmarkFactoryReasonableOrdersHPS(const plugins::Options &opts)
    : LandmarkFactory(opts),
      lm_factory(opts.get<shared_ptr<LandmarkFactory>>("lm_factory")) {
}

void LandmarkFactoryReasonableOrdersHPS::generate_landmarks(const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Building a landmark graph with reasonable orders." << endl;
    }

    lm_graph = lm_factory->compute_lm_graph(task);
    achievers_calculated = lm_factory->achievers_are_calculated();

    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "approx. reasonable orders" << endl;
    }
    approximate_reasonable_orders(task_proxy);
}

void LandmarkFactoryReasonableOrdersHPS::approximate_reasonable_orders(
    const TaskProxy &task_proxy) {
    /*
      Approximate reasonable orders according to Hoffmann et al.

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
        const Landmark &landmark = node_p->get_landmark();
        if (landmark.disjunctive)
            continue;

        if (landmark.is_true_in_goal) {
            for (auto &node2_p : lm_graph->get_nodes()) {
                const Landmark &landmark2 = node2_p->get_landmark();
                if (landmark == landmark2 || landmark2.disjunctive)
                    continue;
                if (interferes(task_proxy, landmark2, landmark)) {
                    edge_add(*node2_p, *node_p, EdgeType::REASONABLE);
                }
            }
        } else {
            // Collect candidates for reasonable orders in "interesting nodes".
            // Use hash set to filter duplicates.
            unordered_set<LandmarkNode *> interesting_nodes(variables_size);
            for (const auto &child : node_p->children) {
                const LandmarkNode &node2_p = *child.first;
                const EdgeType &edge2 = child.second;
                if (edge2 >= EdgeType::GREEDY_NECESSARY) { // found node2_p: node_p ->_gn node2_p
                    for (const auto &p : node2_p.parents) {   // find parent
                        LandmarkNode &parent_node = *(p.first);
                        const EdgeType &edge = p.second;
                        if (parent_node.get_landmark().disjunctive)
                            continue;
                        if (edge >= EdgeType::NATURAL && &parent_node != node_p.get()) {
                            // find predecessors or parent and collect in "interesting nodes"
                            interesting_nodes.insert(&parent_node);
                            collect_ancestors(interesting_nodes, parent_node);
                        }
                    }
                }
            }
            // Insert reasonable orders between those members of "interesting nodes" that interfere
            // with node_p.
            for (LandmarkNode *node2_p : interesting_nodes) {
                const Landmark &landmark2 = node2_p->get_landmark();
                if (landmark == landmark2 || landmark2.disjunctive)
                    continue;
                if (interferes(task_proxy, landmark2, landmark)) {
                    edge_add(*node2_p, *node_p, EdgeType::REASONABLE);
                }
            }
        }
    }
}

bool LandmarkFactoryReasonableOrdersHPS::interferes(
    const TaskProxy &task_proxy, const Landmark &landmark_a,
    const Landmark &landmark_b) const {
    /* Facts a and b interfere (i.e., achieving b before a would mean having to delete b
     and re-achieve it in order to achieve a) if one of the following condition holds:
     1. a and b are mutex
     2. All actions that add a also add e, and e and b are mutex
     3. There is a greedy necessary predecessor x of a, and x and b are mutex
     This is the definition of Hoffmann et al. except that they have one more condition:
     "all actions that add a delete b". However, in our case (SAS+ formalism), this condition
     is the same as 2.
     */
    assert(landmark_a != landmark_b);
    assert(!landmark_a.disjunctive && !landmark_b.disjunctive);

    VariablesProxy variables = task_proxy.get_variables();
    for (const FactPair &lm_fact_b : landmark_b.facts) {
        FactProxy fact_b = variables[lm_fact_b.var].get_fact(lm_fact_b.value);
        for (const FactPair &lm_fact_a : landmark_a.facts) {
            FactProxy fact_a = variables[lm_fact_a.var].get_fact(lm_fact_a.value);
            if (lm_fact_a == lm_fact_b) {
                if (!landmark_a.conjunctive || !landmark_b.conjunctive)
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
            if (landmark_a.conjunctive)
                continue;

            unordered_map<int, int> shared_eff;
            bool init = true;
            const vector<int> &op_or_axiom_ids = get_operators_including_eff(lm_fact_a);
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

void LandmarkFactoryReasonableOrdersHPS::collect_ancestors(
    unordered_set<LandmarkNode *> &result, LandmarkNode &node) {
    /* Returns all ancestors in the landmark graph of landmark node "start" */

    // There could be cycles if use_reasonable == true
    list<LandmarkNode *> open_nodes;
    unordered_set<LandmarkNode *> closed_nodes;
    for (const auto &p : node.parents) {
        LandmarkNode &parent = *(p.first);
        const EdgeType &edge = p.second;
        if (edge >= EdgeType::NATURAL && closed_nodes.count(&parent) == 0) {
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
            if (edge >= EdgeType::NATURAL && closed_nodes.count(&parent) == 0) {
                open_nodes.push_back(&parent);
                closed_nodes.insert(&parent);
                result.insert(&parent);
            }
        }
        open_nodes.pop_front();
    }
}

bool LandmarkFactoryReasonableOrdersHPS::effect_always_happens(
    const VariablesProxy &variables, const EffectsProxy &effects,
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

bool LandmarkFactoryReasonableOrdersHPS::computes_reasonable_orders() const {
    return true;
}

bool LandmarkFactoryReasonableOrdersHPS::supports_conditional_effects() const {
    return lm_factory->supports_conditional_effects();
}

class LandmarkFactoryReasonableOrdersHPSFeature : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryReasonableOrdersHPS> {
public:
    LandmarkFactoryReasonableOrdersHPSFeature() : TypedFeature("lm_reasonable_orders_hps") {
        document_title("HPS Orders");
        document_synopsis(
            "Adds reasonable orders described in the following paper" +
            utils::format_journal_reference(
                {"Jörg Hoffmann", "Julie Porteous", "Laura Sebastia"},
                "Ordered Landmarks in Planning",
                "https://jair.org/index.php/jair/article/view/10390/24882",
                "Journal of Artificial Intelligence Research",
                "22",
                "215-278",
                "2004"));

        document_note(
            "Obedient-reasonable orders",
            "Hoffmann et al. (2004) suggest obedient-reasonable orders in "
            "addition to reasonable orders. Obedient-reasonable orders were "
            "later also used by the LAMA planner (Richter and Westphal, 2010). "
            "They are \"reasonable orders\" under the assumption that all "
            "(non-obedient) reasonable orders are actually \"natural\", i.e., "
            "every plan obeys the reasonable orders. We observed "
            "experimentally that obedient-reasonable orders have minimal "
            "effect on the performance of LAMA (Büchner et al., 2023) and "
            "decided to remove them in issue1089.");

        add_option<shared_ptr<LandmarkFactory>>("lm_factory");
        add_landmark_factory_options_to_feature(*this);

        // TODO: correct?
        document_language_support(
            "conditional_effects",
            "supported if subcomponent supports them");
    }
};

static plugins::FeaturePlugin<LandmarkFactoryReasonableOrdersHPSFeature> _plugin;
}
