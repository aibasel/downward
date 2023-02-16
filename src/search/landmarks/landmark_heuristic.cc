#include "landmark_heuristic.h"

#include "landmark.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"

using namespace std;

namespace landmarks {
static bool landmark_is_interesting(
    const State &state, const BitsetView &reached,
    const landmarks::LandmarkNode &lm_node, bool all_lms_reached) {
    /*
      We consider a landmark interesting in two (exclusive) cases:
      (1) If all landmarks are reached and the landmark must hold in the goal
          but does not hold in the current state.
      (2) If it has not been reached before and all its parents are reached.
    */

    if (all_lms_reached) {
        const Landmark &landmark = lm_node.get_landmark();
        return landmark.is_true_in_goal && !landmark.is_true_in_state(state);
    } else {
        return !reached.test(lm_node.get_id()) &&
               all_of(lm_node.parents.begin(), lm_node.parents.end(),
                      [&](const pair<LandmarkNode *, EdgeType> parent) {
                          return reached.test(parent.first->get_id());
                      });
    }
}

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
    lm_status_manager =
        utils::make_unique_ptr<LandmarkStatusManager>(*lm_graph);

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main
           task in cases where it's compatible. See issue564. */
        successor_generator =
            utils::make_unique_ptr<successor_generator::SuccessorGenerator>(
                task_proxy);
    }
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
    const State &state, const BitsetView &reached) {
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

    bool all_landmarks_reached = true;
    for (int i = 0; i < reached.size(); ++i) {
        if (!reached.test(i)) {
            all_landmarks_reached = false;
            break;
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
                    state, reached, *lm_node, all_landmarks_reached)) {
                if (lm_node->get_landmark().disjunctive) {
                    preferred_operators_disjunctive.push_back(op_id);
                } else {
                    preferred_operators_simple.push_back(op_id);
                }
            }
        }
    }

    OperatorsProxy operators = task_proxy.get_operators();
    if (preferred_operators_simple.empty()) {
        for (OperatorID op_id : preferred_operators_disjunctive) {
            set_preferred(operators[op_id]);
        }
    } else {
        for (OperatorID op_id : preferred_operators_simple) {
            set_preferred(operators[op_id]);
        }
    }
}

int LandmarkHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    lm_status_manager->update_lm_status(ancestor_state);
    int h = get_heuristic_value(state);

    if (use_preferred_operators) {
        BitsetView reached_lms =
            lm_status_manager->get_reached_landmarks(ancestor_state);
        generate_preferred_operators(state, reached_lms);
    }

    return h;
}

void LandmarkHeuristic::notify_initial_state(const State &initial_state) {
    lm_status_manager->process_initial_state(initial_state, log);
}

void LandmarkHeuristic::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    lm_status_manager->process_state_transition(parent_state, op_id, state);
    if (cache_evaluator_values) {
        /* TODO:  It may be more efficient to check that the reached landmark
            set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
}

void LandmarkHeuristic::add_options_to_feature(plugins::Feature &feature) {
    feature.add_option<shared_ptr<LandmarkFactory>>(
        "lm_factory",
        "the set of landmarks to use for this heuristic. "
        "The set of landmarks can be specified here, "
        "or predefined (see LandmarkFactory).");
    feature.add_option<bool>(
        "pref",
        "identify preferred operators (see OptionCaveats#"
        "Using_preferred_operators_with_landmark_heuristics)",
        "false");
    Heuristic::add_options_to_feature(feature);

    feature.document_property("preferred operators",
                              "yes (if enabled; see ``pref`` option)");
}
}
