#include "landmark_factory_reasonable_orders_hps.h"

#include "landmark.h"
#include "landmark_graph.h"

#include "util.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"

#include <ranges>

using namespace std;
namespace landmarks {
LandmarkFactoryReasonableOrdersHPS::LandmarkFactoryReasonableOrdersHPS(
    const shared_ptr<LandmarkFactory> &lm_factory,
    utils::Verbosity verbosity)
    : LandmarkFactory(verbosity),
      landmark_factory(lm_factory) {
}

void LandmarkFactoryReasonableOrdersHPS::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    if (log.is_at_least_normal()) {
        log << "Building a landmark graph with reasonable orders." << endl;
    }

    landmark_graph = landmark_factory->compute_landmark_graph(task);
    achievers_calculated = landmark_factory->achievers_are_calculated();

    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "approx. reasonable orders" << endl;
    }
    approximate_reasonable_orderings(task_proxy);
}

void LandmarkFactoryReasonableOrdersHPS::approximate_goal_orderings(
    const TaskProxy &task_proxy, LandmarkNode &node) const {
    const Landmark &landmark = node.get_landmark();
    assert(landmark.is_true_in_goal);
    for (const auto &other : *landmark_graph) {
        const Landmark &other_landmark = other->get_landmark();
        if (landmark == other_landmark || other_landmark.type == DISJUNCTIVE) {
            continue;
        }
        if (interferes(task_proxy, other_landmark, landmark)) {
            add_or_replace_ordering_if_stronger(
                *other, node, OrderingType::REASONABLE);
        }
    }
}

static void collect_ancestors(unordered_set<LandmarkNode *> &result,
                              const LandmarkNode &node) {
    for (const auto &[parent, type] : node.parents) {
        if (type >= OrderingType::NATURAL && !result.contains(parent)) {
            result.insert(parent);
            collect_ancestors(result, *parent);
        }
    }
}

static unordered_set<LandmarkNode *> collect_reasonable_ordering_candidates(
    const LandmarkNode &node) {
    unordered_set<LandmarkNode *> interesting_nodes;
    for (const auto &[child, type] : node.children) {
        if (type >= OrderingType::GREEDY_NECESSARY) {
            // Found a landmark such that `node` ->_gn `child`.
            for (const auto &[parent, parent_type]: child->parents) {
                if (parent->get_landmark().type == DISJUNCTIVE) {
                    continue;
                }
                if (parent_type >= OrderingType::NATURAL && *parent != node) {
                    /* Find predecessors or parent and collect in
                       `interesting nodes`. */
                    interesting_nodes.insert(parent);
                    collect_ancestors(interesting_nodes, *parent);
                }
            }
        }
    }
    return interesting_nodes;
}

/* Insert reasonable orderings between the `candidates` that interfere
   with `landmark`. */
void LandmarkFactoryReasonableOrdersHPS::insert_reasonable_orderings(
    const TaskProxy &task_proxy,
    const unordered_set<LandmarkNode *> &candidates,
    LandmarkNode &node, const Landmark &landmark) const {
    for (LandmarkNode *other : candidates) {
        const Landmark &other_landmark = other->get_landmark();
        if (landmark == other_landmark || other_landmark.type == DISJUNCTIVE) {
            continue;
        }
        if (interferes(task_proxy, other_landmark, landmark)) {
            add_or_replace_ordering_if_stronger(
                *other, node, OrderingType::REASONABLE);
        }
    }
}

/*
  Approximate reasonable orderings according to Hoffmann et al. (JAIR 2004):

  If `landmark` holds in the goal, any other landmark which interferes
  with it is reasonably ordered before it. Otherwise, if `landmark` is a
  greedy-necessary predecessor of another landmark, and there is
  another predecessor `parent` of that other landmark (`candidates`),
  then `parent` and all predecessors of `parent` are ordered reasonably
  before `landmark` if they interfere with it.
*/
void LandmarkFactoryReasonableOrdersHPS::approximate_reasonable_orderings(
    const TaskProxy &task_proxy) {
    State initial_state = task_proxy.get_initial_state();
    for (const auto &node : *landmark_graph) {
        const Landmark &landmark = node->get_landmark();
        if (landmark.type == DISJUNCTIVE) {
            continue;
        }

        if (landmark.is_true_in_goal) {
            approximate_goal_orderings(task_proxy, *node);
        } else {
            unordered_set<LandmarkNode *> candidates =
                collect_reasonable_ordering_candidates(*node);
            insert_reasonable_orderings(
                task_proxy, candidates, *node, landmark);
        }
    }
}

struct EffectConditionSet {
    int value;
    utils::HashSet<FactPair> conditions;
};

static unordered_map<int, EffectConditionSet> compute_effect_conditions_by_variable(
    const EffectsProxy &effects) {
    // Variables that occur in multiple effects with different values.
    unordered_set<int> nogood_effect_vars;
    unordered_map<int, EffectConditionSet> effect_conditions_by_variable;
    for (EffectProxy effect : effects) {
        EffectConditionsProxy effect_conditions = effect.get_conditions();
        auto [var, value] = effect.get_fact().get_pair();
        if (effect_conditions.empty() || nogood_effect_vars.contains(var)) {
            continue;
        }
        if (effect_conditions_by_variable.contains(var)) {
            // We have seen `var` in an effect before.
            int old_effect_value = effect_conditions_by_variable[var].value;
            if (old_effect_value != value) {
                nogood_effect_vars.insert(var);
                continue;
            }

            // Add more conditions to this previously seen effect.
            for (FactProxy effect_condition : effect_conditions) {
                effect_conditions_by_variable[var].conditions.insert(
                    effect_condition.get_pair());
            }
        } else {
            // We have not seen this effect `var` before, so we add a new entry.
            utils::HashSet<FactPair> conditions;
            conditions.reserve(effect_conditions.size());
            for (FactProxy effect_condition : effect_conditions) {
                conditions.insert(effect_condition.get_pair());
            }
            effect_conditions_by_variable[var] = {value, move(conditions)};
        }
    }
    for (int var : nogood_effect_vars) {
        effect_conditions_by_variable.erase(var);
    }
    return effect_conditions_by_variable;
}

static unordered_map<int, unordered_set<int>> get_conditions_by_variable(
    const EffectConditionSet &effect_conditions) {
    unordered_map<int, unordered_set<int>> conditions_by_var;
    for (auto [var, value] : effect_conditions.conditions) {
        conditions_by_var[var].insert(value);
    }
    return conditions_by_var;
}

static bool effect_always_happens(
    int effect_var, const EffectConditionSet &effect_conditions,
    const VariablesProxy &variables) {
    unordered_map<int, unordered_set<int>> conditions_by_var =
        get_conditions_by_variable(effect_conditions);

    /*
      The effect always happens if for all variables with effect conditions it
      holds that (1) the effect triggers for all possible values in their domain
      or (2) the variable of the condition is the variable of the effect and the
      effect triggers for all other non-effect values in their domain.
    */
    for (auto &[conditions_var, values] : conditions_by_var) {
        size_t domain_size = variables[effect_var].get_domain_size();
        assert(values.size() <= domain_size);
        if (effect_var == conditions_var) {
            /* Extending the `values` with the `effect_conditions.value`
               completes the domain if the effect always happens. */
            values.insert(effect_conditions.value);
        }
        if (values.size() < domain_size) {
            return false;
        }
    }
    return true;
}

/*
  Test whether the condition of a conditional effect is trivial, i.e. always
  true. We test for the simple case that the same effect proposition is
  triggered by a set of conditions of which one will always be true. This is for
  example the case in Schedule, where the effect
      (forall (?oldpaint - colour)
          (when (painted ?x ?oldpaint)
              (not (painted ?x ?oldpaint))))
  is translated by the translator to:
      if oldpaint == blue, then not painted ?x, and
      if oldpaint == red, then not painted ?x, etc.
  Conditional effects that always happen are returned in `always_effects`.
*/
static utils::HashSet<FactPair> get_effects_that_always_happen(
    const VariablesProxy &variables, const EffectsProxy &effects) {
    unordered_map<int, EffectConditionSet> effect_conditions_by_variable =
        compute_effect_conditions_by_variable(effects);

    utils::HashSet<FactPair> always_effects;
    for (const auto &[var, effect_conditions] : effect_conditions_by_variable) {
        if (effect_always_happens(var, effect_conditions, variables)) {
            always_effects.insert(FactPair(var, effect_conditions.value));
        }
    }
    return always_effects;
}

/*
  Insert effects of this operator into `effect` if it is an
  unconditional effect or a conditional effect that is sure to happen.
  (Such "trivial" conditions can arise due to our translator, e.g. in
  Schedule. There, the same effect is conditioned on a disjunction of
  conditions of which one will always be true. We test for a simple kind
  of these trivial conditions here.)
*/
static utils::HashSet<FactPair> get_effects_on_other_variables(
    const TaskProxy &task_proxy, int op_or_axiom_id, int var_id) {
    EffectsProxy effects =
        get_operator_or_axiom(task_proxy, op_or_axiom_id).get_effects();
    utils::HashSet<FactPair> trivially_conditioned_effects =
        get_effects_that_always_happen(task_proxy.get_variables(), effects);
    utils::HashSet<FactPair> next_effect;
    for (const EffectProxy &effect : effects) {
        FactPair atom = effect.get_fact().get_pair();
        if ((effect.get_conditions().empty() && atom.var != var_id) ||
            trivially_conditioned_effects.contains(atom)) {
            next_effect.insert(atom);
        }
    }
    return next_effect;
}

utils::HashSet<FactPair> LandmarkFactoryReasonableOrdersHPS::get_shared_effects_of_achievers(
    const FactPair &atom, const TaskProxy &task_proxy) const {
    utils::HashSet<FactPair> shared_effects;

    // Intersect effects of operators that achieve `atom` one by one.
    bool init = true;
    for (int op_or_axiom_id : get_operators_including_effect(atom)) {
        utils::HashSet<FactPair> effect = get_effects_on_other_variables(
            task_proxy, op_or_axiom_id, atom.var);

        if (init) {
            swap(shared_effects, effect);
            init = false;
        } else {
            shared_effects = get_intersection(shared_effects, effect);
        }

        if (shared_effects.empty()) {
            assert(!init);
            break;
        }
    }
    return shared_effects;
}

/*
  An atom B interferes with another atom A if achieving A is impossible while B
  holds. This can be either because B may not be true when applying any operator
  that achieves A (because it cannot hold at the same time as these
  preconditions) or because the effects of operators that achieve A delete B.
  Specifically, B interferes with A if one of the following conditions holds:
   1. A and B are mutex.
   2. All operators that add A also add E, and E and B are mutex.
   3. There is a greedy-necessary predecessor X of A, and X and B are mutex.
  This is the definition of Hoffmann et al. except that they have one more
  condition: "All actions that add A delete B." However, in our case (SAS+
  formulation), this condition is the same as 2.
*/
bool LandmarkFactoryReasonableOrdersHPS::interferes(
    const VariablesProxy &variables, const TaskProxy &task_proxy,
    const Landmark &landmark_a, const FactPair &atom_a, const FactProxy &a,
    const FactProxy &b) const {
    // 1. A and B are mutex.
    if (a.is_mutex(b)) {
        return true;
    }

    /*
      2. All operators reaching A have a shared effect E, and E and B are mutex.
      Skip this case for conjunctive landmarks A, as they are typically achieved
      through a sequence of operators successively adding the parts of A.
    */
    if (landmark_a.type == CONJUNCTIVE) {
        return false;
    }
    utils::HashSet<FactPair> shared_effects =
        get_shared_effects_of_achievers(atom_a, task_proxy);
    return ranges::any_of(shared_effects, [&](const FactPair &atom) {
                              const FactProxy &e = variables[atom.var].get_fact(atom.value);
                              return e != a && e != b && e.is_mutex(b);
                          });

    /*
      Experimentally commenting this out -- see issue202.
      TODO: This code became unreachable and no longer works after
       all the refactorings we did recently. Maybe we should just remove it?
    // Case 3: There exists an atom X inconsistent with B such that X->_gn A.
    for (const auto &parent : node_a->parents) {
        const LandmarkNode &node = *parent.first;
        edge_type edge = parent.second;
        for (const FactPair &parent_prop : node.facts) {
            const FactProxy &parent_prop_fact =
                variables[parent_prop.var].get_fact(parent_prop.value);
            if (edge >= greedy_necessary &&
                parent_prop_fact != fact_b &&
                parent_prop_fact.is_mutex(fact_b)) {
                return true;
            }
        }
    }
    */
}

bool LandmarkFactoryReasonableOrdersHPS::interferes(
    const TaskProxy &task_proxy, const Landmark &landmark_a,
    const Landmark &landmark_b) const {
    assert(landmark_a != landmark_b);
    assert(landmark_a.type != DISJUNCTIVE);
    assert(landmark_b.type != DISJUNCTIVE);

    VariablesProxy variables = task_proxy.get_variables();
    for (const FactPair &atom_b : landmark_b.atoms) {
        FactProxy b = variables[atom_b.var].get_fact(atom_b.value);
        for (const FactPair &atom_a : landmark_a.atoms) {
            FactProxy a = variables[atom_a.var].get_fact(atom_a.value);
            if (atom_a == atom_b) {
                if (landmark_a.type == CONJUNCTIVE &&
                    landmark_b.type == CONJUNCTIVE) {
                    continue;
                }
                return false;
            }
            if (interferes(variables, task_proxy, landmark_a, atom_a, a, b)) {
                return true;
            }
        }
    }
    return false;
}

bool LandmarkFactoryReasonableOrdersHPS::supports_conditional_effects() const {
    return landmark_factory->supports_conditional_effects();
}

class LandmarkFactoryReasonableOrdersHPSFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryReasonableOrdersHPS> {
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
            "Hoffmann et al. (2004) suggest obedient-reasonable orderings in "
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

    virtual shared_ptr<LandmarkFactoryReasonableOrdersHPS> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryReasonableOrdersHPS>(
            opts.get<shared_ptr<LandmarkFactory>>("lm_factory"),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryReasonableOrdersHPSFeature> _plugin;
}
