#include "landmark_factory.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/memory.h"
#include "../utils/timer.h"

#include <fstream>
#include <limits>

using namespace std;

namespace landmarks {
LandmarkFactory::LandmarkFactory(utils::Verbosity verbosity)
    : log(utils::get_log_for_verbosity(verbosity)), lm_graph(nullptr) {
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

    lm_graph = make_shared<LandmarkGraph>();

    TaskProxy task_proxy(*task);
    generate_operators_lookups(task_proxy);
    generate_landmarks(task);

    if (log.is_at_least_normal()) {
        log << "Landmarks generation time: " << lm_generation_timer << endl;
        if (lm_graph->get_num_landmarks() == 0) {
            if (log.is_warning()) {
                log << "Warning! No landmarks found. Task unsolvable?" << endl;
            }
        } else {
            log << "Discovered " << lm_graph->get_num_landmarks()
                << " landmarks, of which " << lm_graph->get_num_disjunctive_landmarks()
                << " are disjunctive and "
                << lm_graph->get_num_conjunctive_landmarks() << " are conjunctive." << endl;
            log << lm_graph->get_num_edges() << " edges" << endl;
        }
    }

    if (log.is_at_least_debug()) {
        dump_landmark_graph(task_proxy, *lm_graph, log);
    }
    return lm_graph;
}

bool LandmarkFactory::is_landmark_precondition(
    const OperatorProxy &op, const Landmark &landmark) const {
    /* Test whether the landmark is used by the operator as a precondition.
    A disjunctive landmarks is used if one of its disjuncts is used. */
    for (FactProxy pre : op.get_preconditions()) {
        for (const FactPair &lm_fact : landmark.facts) {
            if (pre.get_pair() == lm_fact)
                return true;
        }
    }
    return false;
}

void LandmarkFactory::edge_add(LandmarkNode &from, LandmarkNode &to,
                               EdgeType type) {
    /* Adds an edge in the landmarks graph. If an edge between the same
       landmarks is already present, the stronger edge type wins. */
    assert(&from != &to);

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
        if (log.is_at_least_debug()) {
            log << "added parent with address " << &from << endl;
        }
    }
    assert(from.children.find(&to) != from.children.end());
    assert(to.parents.find(&from) != to.parents.end());
}

void LandmarkFactory::discard_all_orderings() {
    if (log.is_at_least_normal()) {
        log << "Removing all orderings." << endl;
    }
    for (auto &node : lm_graph->get_nodes()) {
        node->children.clear();
        node->parents.clear();
    }
}

void LandmarkFactory::generate_operators_lookups(const TaskProxy &task_proxy) {
    /* Build datastructures for efficient landmark computation. Map propositions
    to the operators that achieve them or have them as preconditions */

    VariablesProxy variables = task_proxy.get_variables();
    operators_eff_lookup.resize(variables.size());
    for (VariableProxy var : variables) {
        operators_eff_lookup[var.get_id()].resize(var.get_domain_size());
    }
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorProxy op : operators) {
        const EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                get_operator_or_axiom_id(op));
        }
    }
    for (OperatorProxy axiom : task_proxy.get_axioms()) {
        const EffectsProxy effects = axiom.get_effects();
        for (EffectProxy effect : effects) {
            const FactProxy effect_fact = effect.get_fact();
            operators_eff_lookup[effect_fact.get_variable().get_id()][effect_fact.get_value()].push_back(
                get_operator_or_axiom_id(axiom));
        }
    }
}

void add_landmark_factory_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}

tuple<utils::Verbosity>
get_landmark_factory_arguments_from_options(
    const plugins::Options &opts) {
    return utils::get_log_arguments_from_options(opts);
}

void add_use_orders_option_to_feature(plugins::Feature &feature) {
    feature.add_option<bool>(
        "use_orders",
        "use orders between landmarks",
        "true");
}

bool get_use_orders_arguments_from_options(
    const plugins::Options &opts) {
    return opts.get<bool>("use_orders");
}

void add_only_causal_landmarks_option_to_feature(
    plugins::Feature &feature) {
    feature.add_option<bool>(
        "only_causal_landmarks",
        "keep only causal landmarks",
        "false");
}

bool get_only_causal_landmarks_arguments_from_options(
    const plugins::Options &opts) {
    return opts.get<bool>("only_causal_landmarks");
}

static class LandmarkFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<LandmarkFactory> {
public:
    LandmarkFactoryCategoryPlugin() : TypedCategoryPlugin("LandmarkFactory") {
        document_synopsis(
            "A landmark factory specification is either a newly created "
            "instance or a landmark factory that has been defined previously. "
            "This page describes how one can specify a new landmark factory "
            "instance. For re-using landmark factories, see "
            "OptionSyntax#Landmark_Predefinitions.");
        allow_variable_binding();
    }
}
_category_plugin;
}
