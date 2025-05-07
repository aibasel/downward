#include "landmark_factory.h"

#include "landmark.h"
#include "landmark_graph.h"
#include "util.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/timer.h"

#include <fstream>
#include <memory>


using namespace std;

namespace landmarks {
LandmarkFactory::LandmarkFactory(utils::Verbosity verbosity)
    : log(get_log_for_verbosity(verbosity)), landmark_graph(nullptr) {
}

void LandmarkFactory::resize_operators_providing_effect(
    const TaskProxy &task_proxy) {
    VariablesProxy variables = task_proxy.get_variables();
    operators_providing_effect.resize(variables.size());
    for (const VariableProxy &var : variables) {
        operators_providing_effect[var.get_id()].resize(var.get_domain_size());
    }
}

void LandmarkFactory::add_operator_or_axiom_providing_effects(
    const OperatorProxy &op_or_axiom) {
    EffectsProxy effects = op_or_axiom.get_effects();
    for (EffectProxy effect : effects) {
        auto [var, value] = effect.get_fact().get_pair();
        operators_providing_effect[var][value].push_back(
            get_operator_or_axiom_id(op_or_axiom));
    }
}

/* Build datastructures for efficient landmark computation: Map propositions to
   the operators and axioms that achieve them. */
void LandmarkFactory::compute_operators_providing_effect(
    const TaskProxy &task_proxy) {
    resize_operators_providing_effect(task_proxy);
    for (const OperatorProxy &op : task_proxy.get_operators()) {
        add_operator_or_axiom_providing_effects(op);
    }
    for (const OperatorProxy &axiom : task_proxy.get_axioms()) {
        add_operator_or_axiom_providing_effects(axiom);
    }
}

static bool weaker_ordering_exists(
    LandmarkNode &from, LandmarkNode &to, OrderingType type) {
    unordered_map<LandmarkNode *, OrderingType>::iterator it;
    if (from.children.size() < to.parents.size()) {
        it = from.children.find(&to);
        if (it == from.children.end()) {
            return false;
        }
    } else {
        it = to.parents.find(&from);
        if (it == to.parents.end()) {
            return false;
        }
    }
    return it->second < type;
}

static void remove_ordering(LandmarkNode &from, LandmarkNode &to) {
    assert(from.children.contains(&to));
    assert(to.parents.contains(&from));
    from.children.erase(&to);
    to.parents.erase(&from);
}

void LandmarkFactory::add_ordering(
    LandmarkNode &from, LandmarkNode &to, OrderingType type) const {
    assert(!from.children.contains(&to));
    assert(!to.parents.contains(&from));
    from.children.emplace(&to, type);
    to.parents.emplace(&from, type);
    if (log.is_at_least_debug()) {
        log << "added parent with address " << &from << endl;
    }
}

/* Adds an ordering in the landmark graph. If an ordering between the same
   landmarks is already present, the stronger ordering type wins. */
void LandmarkFactory::add_or_replace_ordering_if_stronger(
    LandmarkNode &from, LandmarkNode &to, OrderingType type) const {
    if (weaker_ordering_exists(from, to, type)) {
        remove_ordering(from, to);
    }
    if (!from.children.contains(&to)) {
        add_ordering(from, to, type);
    }
    assert(from.children.contains(&to));
    assert(to.parents.contains(&from));
}

void LandmarkFactory::discard_all_orderings() const {
    if (log.is_at_least_normal()) {
        log << "Removing all orderings." << endl;
    }
    for (const auto &node : *landmark_graph) {
        node->children.clear();
        node->parents.clear();
    }
}

void LandmarkFactory::log_landmark_graph_info(
    const TaskProxy &task_proxy,
    const utils::Timer &landmark_generation_timer) const {
    if (log.is_at_least_normal()) {
        log << "Landmarks generation time: "
            << landmark_generation_timer << endl;
        if (landmark_graph->get_num_landmarks() == 0) {
            if (log.is_warning()) {
                log << "Warning! No landmarks found. Task unsolvable?" << endl;
            }
        } else {
            log << "Discovered " << landmark_graph->get_num_landmarks()
                << " landmarks, of which "
                << landmark_graph->get_num_disjunctive_landmarks()
                << " are disjunctive and "
                << landmark_graph->get_num_conjunctive_landmarks()
                << " are conjunctive." << endl;
            log << "There are " << landmark_graph->get_num_orderings()
                << " landmark orderings." << endl;
        }
    }

    if (log.is_at_least_debug()) {
        dump_landmark_graph(task_proxy, *landmark_graph, log);
    }
}

/*
  Note: To allow reusing landmark graphs, we use the following temporary
  solution.

  Landmark factories cache the first landmark graph they compute, so each call
  to this function returns the same graph. Asking for landmark graphs of
  different tasks is an error and will exit with SEARCH_UNSUPPORTED.

  If you want to compute different landmark graphs for different Exploration
  objects, you have to use separate landmark factories.

  This solution remains temporary as long as the question of when and how to
  reuse landmark graphs is open.

  As all heuristics will work on task transformations in the future, this
  function will also get access to a TaskProxy. Then we need to ensure that the
  TaskProxy used by the Exploration object is the same as the TaskProxy object
  passed to this function.
*/
shared_ptr<LandmarkGraph> LandmarkFactory::compute_landmark_graph(
    const shared_ptr<AbstractTask> &task) {
    if (landmark_graph) {
        if (landmark_graph_task != task.get()) {
            cerr << "LandmarkFactory was asked to compute landmark graphs for "
                 << "two different tasks. This is currently not supported."
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
        }
        return landmark_graph;
    }
    landmark_graph_task = task.get();
    utils::Timer landmark_generation_timer;

    landmark_graph = make_shared<LandmarkGraph>();

    TaskProxy task_proxy(*task);
    compute_operators_providing_effect(task_proxy);
    generate_landmarks(task);

    log_landmark_graph_info(task_proxy, landmark_generation_timer);
    return landmark_graph;
}

void add_landmark_factory_options_to_feature(plugins::Feature &feature) {
    utils::add_log_options_to_feature(feature);
}

tuple<utils::Verbosity> get_landmark_factory_arguments_from_options(
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
