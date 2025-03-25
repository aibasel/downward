#include "landmark_factory_zhu_givan.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <unordered_map>
#include <utility>

using namespace std;

namespace landmarks {
LandmarkFactoryZhuGivan::LandmarkFactoryZhuGivan(
    bool use_orders, utils::Verbosity verbosity)
    : LandmarkFactoryRelaxation(verbosity),
      use_orders(use_orders) {
}

void LandmarkFactoryZhuGivan::generate_relaxed_landmarks(
    const shared_ptr<AbstractTask> &task, Exploration &) {
    TaskProxy task_proxy(*task);
    if (log.is_at_least_normal()) {
        log << "Generating landmarks using Zhu/Givan label propagation" << endl;
    }

    compute_triggers(task_proxy);
    PropositionLayer last_prop_layer =
        build_relaxed_plan_graph_with_labels(task_proxy);
    extract_landmarks(task_proxy, last_prop_layer);

    /* TODO: Ensure that landmark orderings are not even added if
        `use_orders` is false. */
    if (!use_orders) {
        discard_all_orderings();
    }
}

/*
  Check if any goal atom is unreachable in the delete relaxation. If so, we
  create a graph with just this atom as landmark and empty achievers to signal
  to the heuristic that the initial state as a dead-end.
*/
bool LandmarkFactoryZhuGivan::goal_is_reachable(
    const TaskProxy &task_proxy, const PropositionLayer &prop_layer) const {
    for (FactProxy goal : task_proxy.get_goals()) {
        if (!prop_layer[goal.get_variable().get_id()][goal.get_value()].reached()) {
            if (log.is_at_least_normal()) {
                log << "Problem not solvable, even if relaxed." << endl;
            }
            Landmark landmark({goal.get_pair()}, false, false, true);
            landmark_graph->add_landmark(move(landmark));
            return false;
        }
    }
    return true;
}

LandmarkNode *LandmarkFactoryZhuGivan::create_goal_landmark(
    const FactPair &goal) const {
    LandmarkNode *node;
    if (landmark_graph->contains_simple_landmark(goal)) {
        node = &landmark_graph->get_simple_landmark_node(goal);
        node->get_landmark().is_true_in_goal = true;
    } else {
        Landmark landmark({goal}, false, false, true);
        node = &landmark_graph->add_landmark(move(landmark));
    }
    return node;
}

void LandmarkFactoryZhuGivan::extract_landmarks_and_orderings_from_goal_labels(
    const FactPair &goal, const PropositionLayer &prop_layer,
    LandmarkNode *goal_landmark_node) const {
    const PlanGraphNode &goal_node = prop_layer[goal.var][goal.value];
    assert(goal_node.reached());

    for (const FactPair &atom : goal_node.labels) {
        if (atom == goal) {
            // Ignore label on itself.
            continue;
        }

        LandmarkNode *node;
        if (landmark_graph->contains_simple_landmark(atom)) {
            node = &landmark_graph->get_simple_landmark_node(atom);
        } else {
            Landmark landmark({atom}, false, false);
            node = &landmark_graph->add_landmark(move(landmark));
        }
        if (use_orders) {
            assert(!node->parents.contains(goal_landmark_node));
            assert(!goal_landmark_node->children.contains(node));
            add_or_replace_ordering_if_stronger(
                *node, *goal_landmark_node, OrderingType::NATURAL);
        }
    }
}

void LandmarkFactoryZhuGivan::extract_landmarks(
    const TaskProxy &task_proxy,
    const PropositionLayer &last_prop_layer) const {
    if (!goal_is_reachable(task_proxy, last_prop_layer)) {
        return;
    }
    for (FactProxy goal : task_proxy.get_goals()) {
        FactPair goal_atom = goal.get_pair();
        LandmarkNode *node = create_goal_landmark(goal_atom);
        extract_landmarks_and_orderings_from_goal_labels(
            goal_atom, last_prop_layer, node);
    }
}

LandmarkFactoryZhuGivan::PropositionLayer LandmarkFactoryZhuGivan::build_relaxed_plan_graph_with_labels(
    const TaskProxy &task_proxy) const {
    assert(!triggers.empty());

    PropositionLayer current_prop_layer;
    unordered_set<int> triggered(task_proxy.get_operators().size() + task_proxy.get_axioms().size());

    // set initial layer
    State initial_state = task_proxy.get_initial_state();
    VariablesProxy variables = task_proxy.get_variables();
    current_prop_layer.resize(variables.size());
    for (VariableProxy var : variables) {
        int var_id = var.get_id();
        current_prop_layer[var_id].resize(var.get_domain_size());

        // label nodes from initial state
        int value = initial_state[var].get_value();
        current_prop_layer[var_id][value].labels.emplace(var_id, value);

        triggered.insert(triggers[var_id][value].begin(), triggers[var_id][value].end());
    }
    // Operators without preconditions do not propagate labels. So if they have
    // no conditional effects, is only necessary to apply them once. (If they
    // have conditional effects, they will be triggered at later stages again).
    triggered.insert(operators_without_preconditions.begin(),
                     operators_without_preconditions.end());

    bool changes = true;
    while (changes) {
        PropositionLayer next_prop_layer(current_prop_layer);
        unordered_set<int> next_triggered;
        changes = false;
        for (int op_or_axiom_id : triggered) {
            OperatorProxy op = get_operator_or_axiom(task_proxy, op_or_axiom_id);
            if (operator_applicable(op, current_prop_layer)) {
                LandmarkSet changed = apply_operator_and_propagate_labels(
                    op, current_prop_layer, next_prop_layer);
                if (!changed.empty()) {
                    changes = true;
                    for (const FactPair &landmark : changed)
                        next_triggered.insert(
                            triggers[landmark.var][landmark.value].begin(),
                            triggers[landmark.var][landmark.value].end());
                }
            }
        }
        current_prop_layer = next_prop_layer;
        triggered = next_triggered;
    }

    return current_prop_layer;
}

bool LandmarkFactoryZhuGivan::operator_applicable(const OperatorProxy &op,
                                                  const PropositionLayer &state) const {
    // test preconditions
    for (FactProxy fact : op.get_preconditions())
        if (!state[fact.get_variable().get_id()][fact.get_value()].reached())
            return false;
    return true;
}

bool LandmarkFactoryZhuGivan::operator_cond_effect_fires(
    const EffectConditionsProxy &effect_conditions, const PropositionLayer &layer) const {
    for (FactProxy effect_condition : effect_conditions)
        if (!layer[effect_condition.get_variable().get_id()][effect_condition.get_value()].reached())
            return false;
    return true;
}

static LandmarkSet _union(const LandmarkSet &a, const LandmarkSet &b) {
    if (a.size() < b.size())
        return _union(b, a);

    LandmarkSet result = a;

    for (LandmarkSet::const_iterator it = b.begin(); it != b.end(); ++it)
        result.insert(*it);
    return result;
}

static LandmarkSet _intersection(const LandmarkSet &a, const LandmarkSet &b) {
    if (a.size() > b.size())
        return _intersection(b, a);

    LandmarkSet result;

    for (LandmarkSet::const_iterator it = a.begin(); it != a.end(); ++it)
        if (b.find(*it) != b.end())
            result.insert(*it);
    return result;
}

LandmarkSet LandmarkFactoryZhuGivan::union_of_precondition_labels(
    const OperatorProxy &op, const PropositionLayer &current) const {
    LandmarkSet result;

    // TODO This looks like an O(n^2) algorithm where O(n log n) would do, a
    // bit like the Python string concatenation anti-pattern.
    for (FactProxy precondition : op.get_preconditions())
        result = _union(result,
                        current[precondition.get_variable().get_id()][precondition.get_value()].labels);

    return result;
}

LandmarkSet LandmarkFactoryZhuGivan::union_of_condition_labels(
    const EffectConditionsProxy &effect_conditions, const PropositionLayer &current) const {
    LandmarkSet result;
    for (FactProxy effect_condition : effect_conditions)
        result = _union(result, current[effect_condition.get_variable().get_id()][effect_condition.get_value()].labels);

    return result;
}

static bool _propagate_labels(LandmarkSet &labels, const LandmarkSet &new_labels,
                              const FactPair &prop) {
    LandmarkSet old_labels = labels;

    if (!labels.empty()) {
        labels = _intersection(labels, new_labels);
    } else {
        labels = new_labels;
    }
    labels.insert(prop);

    assert(old_labels.empty() || old_labels.size() >= labels.size());
    assert(!labels.empty());
    // test if labels have changed or proposition has just been reached:
    // if it has just been reached:
    // (old_labels.size() == 0) && (labels.size() >= 1)
    // if old_labels.size() == labels.size(), then labels have not been refined
    // by intersection.
    return old_labels.size() != labels.size();
}

LandmarkSet LandmarkFactoryZhuGivan::apply_operator_and_propagate_labels(
    const OperatorProxy &op, const PropositionLayer &current,
    PropositionLayer &next) const {
    assert(operator_applicable(op, current));

    LandmarkSet result;
    LandmarkSet precond_label_union = union_of_precondition_labels(op, current);

    for (EffectProxy effect : op.get_effects()) {
        FactPair effect_fact = effect.get_fact().get_pair();

        if (next[effect_fact.var][effect_fact.value].labels.size() == 1)
            continue;

        if (operator_cond_effect_fires(effect.get_conditions(), current)) {
            const LandmarkSet precond_label_union_with_condeff = _union(
                precond_label_union, union_of_condition_labels(
                    // NOTE: this equals precond_label_union, if effects[i] is
                    // not a conditional effect.
                    effect.get_conditions(), current));

            if (_propagate_labels(next[effect_fact.var][effect_fact.value].labels,
                                  precond_label_union_with_condeff, effect_fact))
                result.insert(effect_fact);
        }
    }

    return result;
}

void LandmarkFactoryZhuGivan::compute_triggers(const TaskProxy &task_proxy) {
    assert(triggers.empty());

    // initialize empty triggers
    VariablesProxy variables = task_proxy.get_variables();
    triggers.resize(variables.size());
    for (size_t i = 0; i < variables.size(); ++i)
        triggers[i].resize(variables[i].get_domain_size());

    // compute triggers
    for (OperatorProxy op : task_proxy.get_operators()) {
        add_operator_to_triggers(op);
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        add_operator_to_triggers(axiom);
    }
}

void LandmarkFactoryZhuGivan::add_operator_to_triggers(const OperatorProxy &op) {
    // Collect possible triggers first.
    LandmarkSet possible_triggers;

    int op_or_axiom_id = get_operator_or_axiom_id(op);
    PreconditionsProxy preconditions = op.get_preconditions();
    for (FactProxy precondition : preconditions)
        possible_triggers.insert(precondition.get_pair());

    for (EffectProxy effect : op.get_effects()) {
        for (FactProxy effect_condition : effect.get_conditions())
            possible_triggers.insert(effect_condition.get_pair());
    }
    if (preconditions.empty())
        operators_without_preconditions.push_back(op_or_axiom_id);

    // Add operator to triggers vector.
    for (const FactPair &landmark : possible_triggers)
        triggers[landmark.var][landmark.value].push_back(op_or_axiom_id);
}

bool LandmarkFactoryZhuGivan::supports_conditional_effects() const {
    return true;
}

class LandmarkFactoryZhuGivanFeature
    : public plugins::TypedFeature<LandmarkFactory, LandmarkFactoryZhuGivan> {
public:
    LandmarkFactoryZhuGivanFeature() : TypedFeature("lm_zg") {
        document_title("Zhu/Givan Landmarks");
        document_synopsis(
            "The landmark generation method introduced by "
            "Zhu & Givan (ICAPS 2003 Doctoral Consortium).");

        add_use_orders_option_to_feature(*this);
        add_landmark_factory_options_to_feature(*this);

        // TODO: Make sure that conditional effects are indeed supported.
        document_language_support(
            "conditional_effects",
            "We think they are supported, but this is not 100% sure.");
    }

    virtual shared_ptr<LandmarkFactoryZhuGivan> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<LandmarkFactoryZhuGivan>(
            get_use_orders_arguments_from_options(opts),
            get_landmark_factory_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<LandmarkFactoryZhuGivanFeature> _plugin;
}
