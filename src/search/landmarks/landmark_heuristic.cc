#include "landmark_heuristic.h"

#include "landmark.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"
#include "../utils/markup.h"

using namespace std;

namespace landmarks {
LandmarkHeuristic::LandmarkHeuristic(
    bool use_preferred_operators,
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity),
      use_preferred_operators(use_preferred_operators),
      successor_generator(nullptr) {
}

void LandmarkHeuristic::initialize(
    const shared_ptr<LandmarkFactory> &lm_factory, bool prog_goal,
    bool prog_gn, bool prog_r) {
    /*
      Actually, we should test if this is the root task or a
      task that *only* transforms costs and/or adds negated axioms.
      However, there is currently no good way to do this, so we use
      this incomplete, slightly less safe test.
    */
    if (task != tasks::g_root_task
        && dynamic_cast<tasks::CostAdaptedTask *>(task.get()) == nullptr
        && dynamic_cast<tasks::DefaultValueAxiomsTask *>(task.get()) == nullptr) {
        cerr << "The landmark heuristics currently only support "
             << "task transformations that modify the operator costs "
             << "or add negated axioms. See issues 845, 686 and 454 "
             << "for details." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    compute_landmark_graph(lm_factory);
    lm_status_manager = make_unique<LandmarkStatusManager>(
        *lm_graph, prog_goal, prog_gn, prog_r);

    initial_landmark_graph_has_cycle_of_natural_orderings =
        landmark_graph_has_cycle_of_natural_orderings();
    if (initial_landmark_graph_has_cycle_of_natural_orderings
        && log.is_at_least_normal()) {
        log << "Landmark graph contains a cycle of natural orderings." << endl;
    }

    if (use_preferred_operators) {
        compute_landmarks_achieved_by_fact();
        /* Ideally, we should reuse the successor generator of the main
           task in cases where it's compatible. See issue564. */
        successor_generator =
            make_unique<successor_generator::SuccessorGenerator>(task_proxy);
    }
}

bool LandmarkHeuristic::landmark_graph_has_cycle_of_natural_orderings() {
    int num_landmarks = lm_graph->get_num_landmarks();
    vector<bool> closed(num_landmarks, false);
    vector<bool> visited(num_landmarks, false);
    for (auto &node : lm_graph->get_nodes()) {
        if (depth_first_search_for_cycle_of_natural_orderings(
                *node, closed, visited)) {
            return true;
        }
    }
    return false;
}

bool LandmarkHeuristic::depth_first_search_for_cycle_of_natural_orderings(
    const LandmarkNode &node, vector<bool> &closed, vector<bool> &visited) {
    int id = node.get_id();
    if (closed[id]) {
        return false;
    } else if (visited[id]) {
        return true;
    }

    visited[id] = true;
    for (auto &child : node.children) {
        if (child.second >= EdgeType::NATURAL) {
            if (depth_first_search_for_cycle_of_natural_orderings(
                    *child.first, closed, visited)) {
                return true;
            }
        }
    }
    closed[id] = true;
    return false;
}

void LandmarkHeuristic::compute_landmark_graph(
    const shared_ptr<LandmarkFactory> &lm_factory) {
    utils::Timer lm_graph_timer;
    if (log.is_at_least_normal()) {
        log << "Generating landmark graph..." << endl;
    }

    lm_graph = lm_factory->compute_lm_graph(task);
    assert(lm_factory->achievers_are_calculated());

    if (log.is_at_least_normal()) {
        log << "Landmark graph generation time: " << lm_graph_timer << endl;
        log << "Landmark graph contains " << lm_graph->get_num_landmarks()
            << " landmarks, of which "
            << lm_graph->get_num_disjunctive_landmarks()
            << " are disjunctive and "
            << lm_graph->get_num_conjunctive_landmarks()
            << " are conjunctive." << endl;
        log << "Landmark graph contains " << lm_graph->get_num_edges()
            << " orderings." << endl;
    }
}

void LandmarkHeuristic::compute_landmarks_achieved_by_fact() {
    for (const auto &node : lm_graph->get_nodes()) {
        const int id = node->get_id();
        const Landmark &lm = node->get_landmark();
        if (lm.conjunctive) {
            /*
              TODO: We currently have no way to declare operators preferred
               based on conjunctive landmarks. We consider this a bug and want
               to fix it in issue1072.
            */
            continue;
        }
        for (const auto &fact_pair : lm.facts) {
            if (landmarks_achieved_by_fact.contains(fact_pair)) {
                landmarks_achieved_by_fact[fact_pair].insert(id);
            } else {
                landmarks_achieved_by_fact[fact_pair] = {id};
            }
        }
    }
}

bool LandmarkHeuristic::operator_is_preferred(
    const OperatorProxy &op, const State &state, ConstBitsetView &future) {
    for (EffectProxy effect : op.get_effects()) {
        if (!does_fire(effect, state)) {
            continue;
        }
        const FactPair fact_pair = effect.get_fact().get_pair();
        if (landmarks_achieved_by_fact.contains(fact_pair)) {
            for (const int id : landmarks_achieved_by_fact[fact_pair]) {
                if (future.test(id)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void LandmarkHeuristic::generate_preferred_operators(
    const State &state, ConstBitsetView &future) {
    // Find operators that achieve future landmarks.
    assert(successor_generator);
    vector<OperatorID> applicable_operators;
    successor_generator->generate_applicable_ops(state, applicable_operators);

    for (const OperatorID op_id : applicable_operators) {
        const OperatorProxy &op = task_proxy.get_operators()[op_id];
        if (operator_is_preferred(op, state, future)) {
            set_preferred(op);
        }
    }
}

int LandmarkHeuristic::compute_heuristic(const State &ancestor_state) {
    /*
      The path-dependent landmark heuristics are somewhat ill-defined for states
      not reachable from the initial state of a planning task. We therefore
      assume here that they are only called for reachable states. Under this
      view it is correct that the heuristic declares all states as dead-ends if
      the landmark graph of the initial state has a cycle of natural orderings.

      In the paper on landmark progression (Büchner et al., 2023) we suggest a
      way to deal with unseen (incl. unreachable) states: These states have zero
      information, i.e., all landmarks are past and none are future. With this
      definition, heuristics should yield 0 for all states without expanded
      paths from the initial state. It would be nice to close the gap between
      theory and implementation, but we currently don't know how.
      TODO: Maybe we can find a cleaner way to deal with this once we have a
       proper understanding of the theory. What component is responsible to
       detect unsolvability due to cycles of natural orderings or other reasons?
       How is this signaled to the heuristic(s)? Is adding an unsatisfiable
       landmark to the landmark graph an option? But this requires changing the
       landmark graph after construction which we try to avoid at the moment on
       the implementation side.
    */
    if (initial_landmark_graph_has_cycle_of_natural_orderings) {
        return DEAD_END;
    }
    int h = get_heuristic_value(ancestor_state);
    if (use_preferred_operators) {
        ConstBitsetView future =
            lm_status_manager->get_future_landmarks(ancestor_state);
        State state = convert_ancestor_state(ancestor_state);
        generate_preferred_operators(state, future);
    }
    return h;
}

void LandmarkHeuristic::notify_initial_state(const State &initial_state) {
    lm_status_manager->progress_initial_state(initial_state);
}

void LandmarkHeuristic::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    lm_status_manager->progress(parent_state, op_id, state);
    if (cache_evaluator_values) {
        /* TODO:  It may be more efficient to check that the past landmark
            set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
}

void add_landmark_heuristic_options_to_feature(
    plugins::Feature &feature, const string &description) {
    feature.document_synopsis(
        "Landmark progression is implemented according to the following paper:"
        + utils::format_conference_reference(
            {"Clemens Büchner", "Thomas Keller", "Salomé Eriksson", "Malte Helmert"},
            "Landmarks Progression in Heuristic Search",
            "https://ai.dmi.unibas.ch/papers/buechner-et-al-icaps2023.pdf",
            "Proceedings of the Thirty-Third International Conference on "
            "Automated Planning and Scheduling (ICAPS 2023)",
            "70-79",
            "AAAI Press",
            "2023"));

    feature.add_option<shared_ptr<LandmarkFactory>>(
        "lm_factory",
        "the set of landmarks to use for this heuristic. "
        "The set of landmarks can be specified here, "
        "or predefined (see LandmarkFactory).");
    feature.add_option<bool>(
        "pref",
        "enable preferred operators (see note below)",
        "false");
    /* TODO: Do we really want these options or should we just always progress
        everything we can? */
    feature.add_option<bool>(
        "prog_goal", "Use goal progression.", "true");
    feature.add_option<bool>(
        "prog_gn", "Use greedy-necessary ordering progression.", "true");
    feature.add_option<bool>(
        "prog_r", "Use reasonable ordering progression.", "true");
    add_heuristic_options_to_feature(feature, description);

    feature.document_property("preferred operators",
                              "yes (if enabled; see ``pref`` option)");
}

tuple<shared_ptr<LandmarkFactory>, bool, bool, bool, bool,
      shared_ptr<AbstractTask>, bool, string, utils::Verbosity>
get_landmark_heuristic_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        make_tuple(
            opts.get<shared_ptr<LandmarkFactory>>("lm_factory"),
            opts.get<bool>("pref"),
            opts.get<bool>("prog_goal"),
            opts.get<bool>("prog_gn"),
            opts.get<bool>("prog_r")),
        get_heuristic_arguments_from_options(opts));
}
}
