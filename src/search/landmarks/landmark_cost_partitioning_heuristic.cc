#include "landmark_cost_partitioning_heuristic.h"

#include "landmark.h"
#include "landmark_cost_assignment.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"
#include "util.h"

#include "../per_state_bitset.h"

#include "../plugins/plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"
#include "../utils/markup.h"

#include <cmath>
#include <limits>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace landmarks {
LandmarkCostPartitioningHeuristic::LandmarkCostPartitioningHeuristic(const plugins::Options &opts)
    : LandmarkHeuristic(opts),
      use_preferred_operators(opts.get<bool>("pref")),
      conditional_effects_supported(
          opts.get<shared_ptr<LandmarkFactory>>("lm_factory")->supports_conditional_effects()),
      successor_generator(nullptr) {
    if (log.is_at_least_normal()) {
        log << "Initializing landmark count heuristic..." << endl;
    }

    /*
      Actually, we should like to test if this is the root task or a
      CostAdapatedTask *of the root task*, but there is currently no good way
      to do this, so we use this incomplete, slightly less safe test.
    */
    if (task != tasks::g_root_task && dynamic_cast<tasks::CostAdaptedTask *>(task.get()) == nullptr) {
        cerr << "The landmark count heuristic currently only supports task "
             << "transformations that modify the operator costs. See issues 845 "
             << "and 686 for details."
             << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    utils::Timer lm_graph_timer;
    if (log.is_at_least_normal()) {
        log << "Generating landmark graph..." << endl;
    }
    shared_ptr<LandmarkFactory> lm_graph_factory = opts.get<shared_ptr<LandmarkFactory>>("lm_factory");

    if (lm_graph_factory->computes_reasonable_orders()) {
        cerr << "Reasonable orderings should not be used for admissible heuristics" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    } else if (task_properties::has_axioms(task_proxy)) {
        cerr << "cost partitioning does not support axioms" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    } else if (task_properties::has_conditional_effects(task_proxy) &&
               !conditional_effects_supported) {
        cerr << "conditional effects not supported by the landmark generation method" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }

    lgraph = lm_graph_factory->compute_lm_graph(task);
    assert(lm_graph_factory->achievers_are_calculated());
    if (log.is_at_least_normal()) {
        log << "Landmark graph generation time: " << lm_graph_timer << endl;
        log << "Landmark graph contains " << lgraph->get_num_landmarks()
            << " landmarks, of which " << lgraph->get_num_disjunctive_landmarks()
            << " are disjunctive and " << lgraph->get_num_conjunctive_landmarks()
            << " are conjunctive." << endl;
        log << "Landmark graph contains " << lgraph->get_num_edges()
            << " orderings." << endl;
    }
    lm_status_manager = utils::make_unique_ptr<LandmarkStatusManager>(*lgraph);

    if (opts.get<bool>("optimal")) {
        lm_cost_assignment =
            utils::make_unique_ptr<LandmarkEfficientOptimalSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph, opts.get<lp::LPSolverType>("lpsolver"));
    } else {
        lm_cost_assignment =
            utils::make_unique_ptr<LandmarkUniformSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph, opts.get<bool>("alm"));
    }

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main task in cases
           where it's compatible. See issue564. */
        successor_generator = utils::make_unique_ptr<successor_generator::SuccessorGenerator>(task_proxy);
    }
}

int LandmarkCostPartitioningHeuristic::get_heuristic_value(const State &ancestor_state) {
    double epsilon = 0.01;
    lm_status_manager->update_lm_status(ancestor_state);

    double h_val = lm_cost_assignment->cost_sharing_h_value(*lm_status_manager);
    if (h_val == numeric_limits<double>::max()) {
        return DEAD_END;
    } else {
        return static_cast<int>(ceil(h_val - epsilon));
    }
}

int LandmarkCostPartitioningHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);

    /*
      Need explicit test to see if state is a goal state. The landmark
      heuristic may compute h != 0 for a goal state if landmarks are
      achieved before their parents in the landmarks graph (because
      they do not get counted as reached in that case). However, we
      must return 0 for a goal state.
    */
    // TODO: I believe this should not be relevant in the admissible case.
    if (task_properties::is_goal_state(task_proxy, state))
        return 0;

    int h = get_heuristic_value(ancestor_state);

    if (use_preferred_operators) {
        BitsetView reached_lms =
            lm_status_manager->get_reached_landmarks(ancestor_state);
        generate_preferred_operators(state, reached_lms);
    }

    return h;
}

void LandmarkCostPartitioningHeuristic::generate_preferred_operators(
    const State &state, const BitsetView &reached) {
    /*
      Find operators that achieve landmark leaves. If a simple landmark can be
      achieved, prefer only operators that achieve simple landmarks. Otherwise,
      prefer operators that achieve disjunctive landmarks, or don't prefer any
      operators if no such landmarks exist at all.

      TODO: Conjunctive landmarks are ignored in *lgraph->get_node(...)*, so
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
            LandmarkNode *lm_node = lgraph->get_node(fact_proxy.get_pair());
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

bool LandmarkCostPartitioningHeuristic::landmark_is_interesting(
    const State &state, const BitsetView &reached,
    LandmarkNode &lm_node, bool all_lms_reached) const {
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

void LandmarkCostPartitioningHeuristic::notify_initial_state(const State &initial_state) {
    lm_status_manager->process_initial_state(initial_state, log);
}

void LandmarkCostPartitioningHeuristic::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    lm_status_manager->process_state_transition(parent_state, op_id, state);
    if (cache_evaluator_values) {
        /* TODO:  It may be more efficient to check that the reached landmark
           set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
}

bool LandmarkCostPartitioningHeuristic::dead_ends_are_reliable() const {
    return true;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    {
        parser.document_synopsis(
            "Landmark cost partitioning heuristic",
            "See the papers" +
            utils::format_conference_reference(
                {"Erez Karpas", "Carmel Domshlak"},
                "Cost-Optimal Planning with Landmarks",
                "https://www.ijcai.org/Proceedings/09/Papers/288.pdf",
                "Proceedings of the 21st International Joint Conference on "
                "Artificial Intelligence (IJCAI 2009)",
                "1728-1733",
                "AAAI Press",
                "2009") +
            "and" +
            utils::format_conference_reference(
                {"Emil Keyder and Silvia Richter and Malte Helmert"},
                "Sound and Complete Landmarks for And/Or Graphs",
                "https://ai.dmi.unibas.ch/papers/keyder-et-al-ecai2010.pdf",
                "Proceedings of the 19th European Conference on Artificial "
                "Intelligence (ECAI 2010)",
                "335-340",
                "IOS Press",
                "2010"));

        parser.add_option<shared_ptr<LandmarkFactory>>(
            "lm_factory",
            "the set of landmarks to use for this heuristic. "
            "The set of landmarks can be specified here, "
            "or predefined (see LandmarkFactory).");
        parser.add_option<bool>(
            "optimal",
            "use optimal (LP-based) cost sharing "
            "(only makes sense with ``admissible=true``)", "false");
        parser.add_option<bool>(
            "pref",
            "identify preferred operators (see OptionCaveats#"
            "Using_preferred_operators_with_the_lmcount_heuristic)",
            "false");
        parser.add_option<bool>("alm", "use action landmarks", "true");
        lp::add_lp_solver_option_to_parser(parser);
        Heuristic::add_options_to_parser(parser);

        parser.document_note(
            "Optimal search",
            "You probably also want to add this heuristic as a lazy_evaluator "
            "in the A* algorithm to improve heuristic estimates.");
        parser.document_note(
            "Note",
            "To use ``optimal=true``, you must build the planner with LP support. "
            "See LPBuildInstructions.");
        // TODO: is the note below relevant for the only-admissible case?
        parser.document_note(
            "Differences to the literature",
            "This heuristic differs from the description in the literature (see "
            "references above) in the set of preferred operators computed. The "
            "original implementation described in the literature computes two "
            "kinds of preferred operators:\n\n"
            "+ If there is an applicable operator that reaches a landmark, all "
            "such operators are preferred.\n"
            "+ If no such operators exist, perform an FF-style relaxed exploration "
            "towards the nearest landmarks (according to the landmark orderings) "
            "and use the preferred operators of this exploration.\n\n\n"
            "Our implementation of the heuristic only considers preferred "
            "operators of the first type and does not include the second type. The "
            "rationale for this change is that it reduces code complexity and "
            "helps more cleanly separate landmark-based and FF-based computations "
            "in LAMA-like planner configurations. In our experiments, only "
            "considering preferred operators of the first type reduces performance "
            "when using the heuristic and its preferred operators in isolation but "
            "improves performance when using this heuristic in conjunction with "
            "the FF heuristic, as in LAMA-like planner configurations.");

        parser.document_language_support(
            "action costs",
            "supported");
        parser.document_language_support(
            "conditional_effects",
            "not supported");
        parser.document_language_support(
            "axioms",
            "not allowed");

        parser.document_property(
            "admissible",
            "yes");
        /* TODO: This was "yes with admissible=true and optimal cost
            partitioning; otherwise no" before. Can we answer this now that
            the heuristic only cares about admissible? */
        parser.document_property(
            "consistent",
            "complicated; needs further thought");
        parser.document_property(
            "safe",
            "yes except on tasks with axioms or on tasks with "
            "conditional effects when using a LandmarkFactory "
            "not supporting them");
        parser.document_property(
            "preferred operators",
            "yes (if enabled; see ``pref`` option)");
    }
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkCostPartitioningHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("lmcp", _parse);
}
