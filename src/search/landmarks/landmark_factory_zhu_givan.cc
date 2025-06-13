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
    PropositionLayer last_layer =
        build_relaxed_plan_graph_with_labels(task_proxy);
    extract_landmarks(task_proxy, last_layer);
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
            Landmark landmark({goal.get_pair()}, ATOMIC, true);
            landmark_graph->add_landmark(move(landmark));
            return false;
        }
    }
    return true;
}

LandmarkNode *LandmarkFactoryZhuGivan::create_goal_landmark(
    const FactPair &goal) const {
    LandmarkNode *node;
    if (landmark_graph->contains_atomic_landmark(goal)) {
        node = &landmark_graph->get_atomic_landmark_node(goal);
        node->get_landmark().is_true_in_goal = true;
    } else {
        Landmark landmark({goal}, ATOMIC, true);
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
        if (landmark_graph->contains_atomic_landmark(atom)) {
            node = &landmark_graph->get_atomic_landmark_node(atom);
        } else {
            Landmark landmark({atom}, ATOMIC);
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

LandmarkFactoryZhuGivan::PropositionLayer LandmarkFactoryZhuGivan::initialize_relaxed_plan_graph(
    const TaskProxy &task_proxy, unordered_set<int> &triggered_ops) const {
    const State &initial_state = task_proxy.get_initial_state();
    const VariablesProxy &variables = task_proxy.get_variables();
    PropositionLayer initial_layer;
    initial_layer.resize(variables.size());
    for (VariableProxy var : variables) {
        int var_id = var.get_id();
        initial_layer[var_id].resize(var.get_domain_size());

        // Label nodes from initial state.
        int value = initial_state[var].get_value();
        initial_layer[var_id][value].labels.emplace(var_id, value);

        triggered_ops.insert(
            triggers[var_id][value].begin(), triggers[var_id][value].end());
    }
    return initial_layer;
}

void LandmarkFactoryZhuGivan::propagate_labels_until_fixed_point_reached(
    const TaskProxy &task_proxy, unordered_set<int> &&triggered_ops,
    PropositionLayer &current_layer) const {
    bool changes = true;
    while (changes) {
        PropositionLayer next_layer(current_layer);
        unordered_set<int> next_triggers;
        changes = false;
        for (int op_or_axiom_id : triggered_ops) {
            const OperatorProxy &op =
                get_operator_or_axiom(task_proxy, op_or_axiom_id);
            if (operator_is_applicable(op, current_layer)) {
                LandmarkSet changed = apply_operator_and_propagate_labels(
                    op, current_layer, next_layer);
                if (!changed.empty()) {
                    changes = true;
                    for (const FactPair &landmark : changed)
                        next_triggers.insert(
                            triggers[landmark.var][landmark.value].begin(),
                            triggers[landmark.var][landmark.value].end());
                }
            }
        }
        swap(current_layer, next_layer);
        swap(triggered_ops, next_triggers);
    }
}


LandmarkFactoryZhuGivan::PropositionLayer LandmarkFactoryZhuGivan::build_relaxed_plan_graph_with_labels(
    const TaskProxy &task_proxy) const {
    assert(!triggers.empty());

    unordered_set<int> triggered_ops(
        task_proxy.get_operators().size() + task_proxy.get_axioms().size());
    PropositionLayer current_layer =
        initialize_relaxed_plan_graph(task_proxy, triggered_ops);

    /*
      Operators without preconditions do not propagate labels. So if they have
      no conditional effects, it is only necessary to apply them once. If they
      have conditional effects, they will be triggered again at later stages.
    */
    triggered_ops.insert(operators_without_preconditions.begin(),
                         operators_without_preconditions.end());
    propagate_labels_until_fixed_point_reached(
        task_proxy, move(triggered_ops), current_layer);
    return current_layer;
}

bool LandmarkFactoryZhuGivan::operator_is_applicable(
    const OperatorProxy &op, const PropositionLayer &state) {
    for (FactProxy atom : op.get_preconditions()) {
        auto [var, value] = atom.get_pair();
        if (!state[var][value].reached()) {
            return false;
        }
    }
    return true;
}

bool LandmarkFactoryZhuGivan::conditional_effect_fires(
    const EffectConditionsProxy &effect_conditions,
    const PropositionLayer &layer) {
    for (FactProxy effect_condition : effect_conditions) {
        auto [var, value] = effect_condition.get_pair();
        if (!layer[var][value].reached()) {
            return false;
        }
    }
    return true;
}

LandmarkSet LandmarkFactoryZhuGivan::union_of_condition_labels(
    const ConditionsProxy &conditions, const PropositionLayer &current) {
    LandmarkSet result;
    for (FactProxy precondition : conditions) {
        auto [var, value] = precondition.get_pair();
        union_inplace(result, current[var][value].labels);
    }
    return result;
}

// Returns whether labels have changed or `atom` has just been reached.
static bool propagate_labels(
    LandmarkSet &labels, const LandmarkSet &new_labels, const FactPair &atom) {
    int old_labels_size = static_cast<int>(labels.size());

    // If this is the first time `atom` is reached, it has an empty label set.
    if (labels.empty()) {
        labels = new_labels;
    } else {
        labels = get_intersection(labels, new_labels);
    }
    // `atom` is a landmark for itself.
    labels.insert(atom);

    /*
      Updates should always reduce the label set (intersection), except in the
      special case where `atom` was reached for the first time.

      It would be more accurate to actually test the superset relationship
      instead of just comparing set sizes. However, doing so requires storing a
      copy of `labels` just to assert this. Also, it's probably reasonable to
      trust the implementation of `get_intersection` used above enough to not
      even assert this at all here.
    */
    int new_labels_size = static_cast<int>(labels.size());
    assert(old_labels_size == 0 || old_labels_size >= new_labels_size);
    return old_labels_size != new_labels_size;
}

LandmarkSet LandmarkFactoryZhuGivan::apply_operator_and_propagate_labels(
    const OperatorProxy &op, const PropositionLayer &current,
    PropositionLayer &next) const {
    assert(operator_is_applicable(op, current));
    LandmarkSet precondition_labels =
        union_of_condition_labels(op.get_preconditions(), current);

    LandmarkSet result;
    for (const EffectProxy &effect : op.get_effects()) {
        FactPair atom = effect.get_fact().get_pair();
        if (next[atom.var][atom.value].labels.size() == 1) {
            // The only landmark for `atom` is `atom` itself.
            continue;
        }
        if (conditional_effect_fires(effect.get_conditions(), current)) {
            LandmarkSet condition_labels =
                union_of_condition_labels(effect.get_conditions(), current);
            union_inplace(condition_labels, precondition_labels);
            bool labels_changed = propagate_labels(
                next[atom.var][atom.value].labels, condition_labels, atom);
            if (labels_changed) {
                result.insert(atom);
            }
        }
    }
    return result;
}

void LandmarkFactoryZhuGivan::compute_triggers(const TaskProxy &task_proxy) {
    assert(triggers.empty());

    // Initialize the data structure.
    const VariablesProxy &variables = task_proxy.get_variables();
    triggers.resize(variables.size());
    for (int i = 0; i < static_cast<int>(variables.size()); ++i) {
        triggers[i].resize(variables[i].get_domain_size());
    }

    for (OperatorProxy op : task_proxy.get_operators()) {
        add_operator_to_triggers(op);
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        add_operator_to_triggers(axiom);
    }
}

void LandmarkFactoryZhuGivan::add_operator_to_triggers(
    const OperatorProxy &op) {
    int op_or_axiom_id = get_operator_or_axiom_id(op);
    const PreconditionsProxy &preconditions = op.get_preconditions();
    for (FactProxy precondition : preconditions) {
        auto [var, value] = precondition.get_pair();
        triggers[var][value].push_back(op_or_axiom_id);
    }
    for (EffectProxy effect : op.get_effects()) {
        for (FactProxy effect_condition : effect.get_conditions()) {
            auto [var, value] = effect_condition.get_pair();
            triggers[var][value].push_back(op_or_axiom_id);
        }
    }
    if (preconditions.empty()) {
        operators_without_preconditions.push_back(op_or_axiom_id);
    }
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
