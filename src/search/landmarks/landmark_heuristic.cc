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
    const plugins::Options &opts)
    : Heuristic(opts),
      use_preferred_operators(opts.get<bool>("pref")),
      successor_generator(nullptr) {
}

void LandmarkHeuristic::initialize(const plugins::Options &opts) {
    /*
      Actually, we should test if this is the root task or a
      CostAdaptedTask *of the root task*, but there is currently no good
      way to do this, so we use this incomplete, slightly less safe test.
    */
    if (task != tasks::g_root_task
        && dynamic_cast<tasks::CostAdaptedTask *>(task.get()) == nullptr) {
        cerr << "The landmark heuristics currently only support "
             << "task transformations that modify the operator costs. "
             << "See issues 845 and 686 for details." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    compute_landmark_graph(opts);
    lm_status_manager = utils::make_unique_ptr<LandmarkStatusManager>(
        *lm_graph, opts.get<bool>("prog_goal"),
        opts.get<bool>("prog_gn"), opts.get<bool>("prog_r"));

    initial_landmark_graph_has_cycle_of_natural_orderings =
        landmark_graph_has_cycle_of_natural_orderings();
    if (initial_landmark_graph_has_cycle_of_natural_orderings
        && log.is_at_least_normal()) {
        log << "Landmark graph contains a cycle of natural orderings." << endl;
    }

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main
           task in cases where it's compatible. See issue564. */
        successor_generator =
            utils::make_unique_ptr<successor_generator::SuccessorGenerator>(
                task_proxy);
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

void LandmarkHeuristic::compute_landmark_graph(const plugins::Options &opts) {
    utils::Timer lm_graph_timer;
    if (log.is_at_least_normal()) {
        log << "Generating landmark graph..." << endl;
    }

    shared_ptr<LandmarkFactory> lm_graph_factory =
        opts.get<shared_ptr<LandmarkFactory>>("lm_factory");
    lm_graph = lm_graph_factory->compute_lm_graph(task);
    assert(lm_graph_factory->achievers_are_calculated());

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

void LandmarkHeuristic::generate_preferred_operators(
    const State &state, ConstBitsetView &future) {
    /*
      Find operators that achieve future landmarks.
      TODO: Conjunctive landmarks are ignored in *lm_graph->get_node(...)*, so
       they are ignored when computing preferred operators. We consider this
       a bug and want to fix it in issue1072.
    */
    assert(successor_generator);
    vector<OperatorID> applicable_operators;
    successor_generator->generate_applicable_ops(state, applicable_operators);

    for (OperatorID op_id : applicable_operators) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            if (!does_fire(effect, state))
                continue;
            FactProxy fact_proxy = effect.get_fact();
            LandmarkNode *lm_node = lm_graph->get_node(fact_proxy.get_pair());
            if (lm_node && future.test(lm_node->get_id())) {
                set_preferred(op);
            }
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

void LandmarkHeuristic::add_options_to_feature(plugins::Feature &feature) {
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
    Heuristic::add_options_to_feature(feature);

    feature.document_property("preferred operators",
                              "yes (if enabled; see ``pref`` option)");
}
}
