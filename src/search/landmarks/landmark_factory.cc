#include "landmark_factory.h"

#include "landmark.h"
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
    : log(utils::get_log_from_options(opts)), lm_graph(nullptr) {
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
    /* Adds an edge in the landmarks graph if there is no contradicting edge (simple measure to
    reduce cycles. If the edge is already present, the stronger edge type wins.
    */
    assert(&from != &to);

    if (type == EdgeType::REASONABLE || type == EdgeType::OBEDIENT_REASONABLE) { // simple cycle test
        if (from.parents.find(&to) != from.parents.end()) { // Edge in opposite direction exists
            if (log.is_at_least_debug()) {
                log << "edge in opposite direction exists" << endl;
            }
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

void LandmarkFactory::mk_acyclic_graph() {
    unordered_set<LandmarkNode *> acyclic_node_set(lm_graph->get_num_landmarks());
    int removed_edges = 0;
    for (auto &node : lm_graph->get_nodes()) {
        if (acyclic_node_set.find(node.get()) == acyclic_node_set.end())
            removed_edges += loop_acyclic_graph(*node, acyclic_node_set);
    }
    // [Malte] Commented out the following assertion because
    // the old method for this is no longer available.
    // assert(acyclic_node_set.size() == number_of_landmarks());
    if (log.is_at_least_normal()) {
        log << "Removed " << removed_edges
            << " reasonable or obedient reasonable orders" << endl;
    }
}

void LandmarkFactory::remove_first_weakest_cycle_edge(
    list<pair<LandmarkNode *, EdgeType>> &path,
    list<pair<LandmarkNode *, EdgeType>>::iterator it) {
    LandmarkNode *from = path.back().first;
    LandmarkNode *to = it->first;
    EdgeType weakest_edge = path.back().second;
    for (list<pair<LandmarkNode *, EdgeType>>::iterator it2 = it;
         it2 != prev(path.end()); ++it2) {
        EdgeType edge = it2->second;
        if (edge < weakest_edge) {
            from = it2->first;
            to = next(it2)->first;
            weakest_edge = edge;
        }
        if (weakest_edge == EdgeType::OBEDIENT_REASONABLE) {
            break;
        }
    }
    /*
      If the weakest ordering in a cycle is natural (or stronger), this
      indicates that the problem at hand is unsolvable. We signal this by
      clearing the first achievers of all landmarks present in that cycle.
      We assert here that (first) achievers were calculated beforehand to make
      sure that this information is not overwritten later on.
    */
    assert(achievers_calculated);
    if (weakest_edge > EdgeType::REASONABLE) {
        for (list<pair<LandmarkNode *, EdgeType>>::iterator it2 = it;
             it2 != path.end(); ++it2) {
            it2->first->get_landmark().first_achievers.clear();
        }
    }
    assert(from->children.find(to) != from->children.end());
    assert(to->parents.find(from) != to->parents.end());
    from->children.erase(to);
    to->parents.erase(from);
}

int LandmarkFactory::loop_acyclic_graph(LandmarkNode &lmn,
                                        unordered_set<LandmarkNode *> &acyclic_node_set) {
    assert(acyclic_node_set.find(&lmn) == acyclic_node_set.end());
    int nr_removed = 0;
    list<pair<LandmarkNode *, EdgeType>> path;
    unordered_set<LandmarkNode *> visited = unordered_set<LandmarkNode *>(lm_graph->get_num_landmarks());
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
            remove_first_weakest_cycle_edge(path, it);
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

void add_landmark_factory_options_to_parser(options::OptionParser &parser) {
    utils::add_log_options_to_parser(parser);
}

void add_use_orders_option_to_parser(
    OptionParser &parser) {
    parser.add_option<bool>("use_orders",
                            "use orders between landmarks",
                            "true");
}

void add_only_causal_landmarks_option_to_parser(
    OptionParser &parser) {
    parser.add_option<bool>("only_causal_landmarks",
                            "keep only causal landmarks",
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
