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
bool LandmarkHeuristic::landmark_is_interesting(
    const State &state, ConstBitsetView &past, ConstBitsetView &future,
    const landmarks::LandmarkNode &lm_node, bool all_lms_reached) {
    bool is_interesting;
    switch (interesting_landmarks) {
    case InterestingIf::LEGACY:
        /*
          We consider a landmark interesting in two (exclusive) cases:
          (1) If all landmarks are reached and the landmark must hold in the
              goal but does not hold in the current state.
          (2) If it has not been reached before and all its parents are reached.
        */
        if (all_lms_reached) {
            const Landmark &landmark = lm_node.get_landmark();
            is_interesting =
                landmark.is_true_in_goal && !landmark.is_true_in_state(state);
        } else {
            is_interesting =
                !past.test(lm_node.get_id())
                && all_of(lm_node.parents.begin(),
                          lm_node.parents.end(),
                          [&](const pair<LandmarkNode *, EdgeType> parent) {
                              return past.test(
                                  parent.first->get_id());
                          });
        }
        break;
    case InterestingIf::FUTURE:
        // We simply consider a landmark interesting if it is future.
        is_interesting = future.test(lm_node.get_id());
        break;
    case InterestingIf::PARENTS_PAST:
        /* We consider a landmark interesting if it is future and all its
           parents are past. */
        is_interesting =
            future.test(lm_node.get_id())
            && all_of(lm_node.parents.begin(), lm_node.parents.end(),
                      [&](const pair<LandmarkNode *, EdgeType> parent) {
                          return past.test(parent.first->get_id());
                      });
        break;
    default:
        ABORT("Unknown landmark *InterestingIf* type.");
    }
    return is_interesting;
}

LandmarkHeuristic::LandmarkHeuristic(
    const plugins::Options &opts)
    : Heuristic(opts),
      use_preferred_operators(opts.get<bool>("pref")),
      prefer_simple_landmarks(opts.get<bool>("prefer_simple_landmarks")),
      interesting_landmarks(opts.get<InterestingIf>("interesting_if")),
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
    const State &state, ConstBitsetView &past, ConstBitsetView &future) {
    /*
      Find operators that achieve landmark leaves. If a simple landmark can be
      achieved, prefer only operators that achieve simple landmarks. Otherwise,
      prefer operators that achieve disjunctive landmarks, or don't prefer any
      operators if no such landmarks exist at all.

      TODO: Conjunctive landmarks are ignored in *lm_graph->get_node(...)*, so
       they are ignored when computing preferred operators. We consider this
       a bug and want to fix it in issue1072.
    */
    assert(successor_generator);
    vector<OperatorID> applicable_operators;
    successor_generator->generate_applicable_ops(state, applicable_operators);
    vector<OperatorID> preferred_operators_simple;
    vector<OperatorID> preferred_operators_disjunctive;

    bool all_landmarks_past = true;
    if (interesting_landmarks == InterestingIf::LEGACY) {
        for (int i = 0; i < past.size(); ++i) {
            if (!past.test(i)) {
                all_landmarks_past = false;
                break;
            }
        }
    }

    for (OperatorID op_id : applicable_operators) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            if (!does_fire(effect, state))
                continue;
            FactProxy fact_proxy = effect.get_fact();
            LandmarkNode *lm_node = lm_graph->get_node(fact_proxy.get_pair());
            if (lm_node && landmark_is_interesting(
                    state, past, future, *lm_node, all_landmarks_past)) {
                if (lm_node->get_landmark().disjunctive) {
                    preferred_operators_disjunctive.push_back(op_id);
                } else {
                    preferred_operators_simple.push_back(op_id);
                }
            }
        }
    }

    OperatorsProxy operators = task_proxy.get_operators();
    if (prefer_simple_landmarks) {
        // Legacy version.
        if (preferred_operators_simple.empty()) {
            for (OperatorID op_id : preferred_operators_disjunctive) {
                set_preferred(operators[op_id]);
            }
        } else {
            for (OperatorID op_id : preferred_operators_simple) {
                set_preferred(operators[op_id]);
            }
        }
    } else {
        // New and improved, maybe.
        for (OperatorID op_id : preferred_operators_simple) {
            set_preferred(operators[op_id]);
        }
        for (OperatorID op_id : preferred_operators_disjunctive) {
            set_preferred(operators[op_id]);
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
        ConstBitsetView past =
            lm_status_manager->get_past_landmarks(ancestor_state);
        ConstBitsetView future =
            lm_status_manager->get_future_landmarks(ancestor_state);
        State state = convert_ancestor_state(ancestor_state);
        generate_preferred_operators(state, past, future);
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
    /* TODO: Remove this option, this is just for experimenting whether using
        the hierarchy has a positive effect on performance. */
    feature.add_option<bool>(
        "prefer_simple_landmarks",
        "If true, only applicable operators leading to one kind of interesting "
        "landmarks are preferred operators, namely those of the highest "
        "hierarchy (simple > disjunctive)."
        "Otherwise, all applicable operators leading to any interesting "
        "landmark are preferred operators.",
        "true");
    /* TODO: Remove this option, this is just for experimenting which variant
        works best in practice. */
    feature.add_option<InterestingIf>(
        "interesting_if",
        "Choose which strategy to use to decide if a landmark is interesting "
        "in order to compute preferred operators or not.");
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

/* TODO: Remove this plugin, this is just for experimenting which variant works
    best in practice. */
static plugins::TypedEnumPlugin<InterestingIf> _enum_plugin({
    {"legacy", "old variant"},
    {"future", "all future landmarks are interesting"},
    {"parents_past",
     "only future landmarks with all parents reached are interesting"},
});
}
