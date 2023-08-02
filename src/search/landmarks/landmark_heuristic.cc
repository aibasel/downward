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
static bool landmark_is_interesting(
    const State &state, ConstBitsetView &past,
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
        return !past.test(lm_node.get_id()) &&
               all_of(lm_node.parents.begin(), lm_node.parents.end(),
                      [&](const pair<LandmarkNode *, EdgeType> parent) {
                          return past.test(parent.first->get_id());
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
    lm_status_manager = utils::make_unique_ptr<LandmarkStatusManager>(
        *lm_graph, opts.get<bool>("prog_goal"),
        opts.get<bool>("prog_gn"), opts.get<bool>("prog_r"));

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
    const State &state, ConstBitsetView &past) {
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
    for (int i = 0; i < past.size(); ++i) {
        if (!past.test(i)) {
            all_landmarks_past = false;
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
                    state, past, *lm_node, all_landmarks_past)) {
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
    int h = get_heuristic_value(ancestor_state);
    if (use_preferred_operators) {
        ConstBitsetView past = lm_status_manager->get_past_landmarks(ancestor_state);
        State state = convert_ancestor_state(ancestor_state);
        generate_preferred_operators(state, past);
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
    /* TODO: Do we really want these options or should we just allways progress
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
