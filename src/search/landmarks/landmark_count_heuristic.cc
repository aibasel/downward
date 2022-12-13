#include "landmark_count_heuristic.h"

#include "landmark.h"
#include "landmark_cost_assignment.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"
#include "util.h"

#include "../option_parser.h"
#include "../per_state_bitset.h"
#include "../plugin.h"

#include "../lp/lp_solver.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../tasks/cost_adapted_task.h"
#include "../tasks/root_task.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cmath>
#include <limits>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace landmarks {
LandmarkCountHeuristic::LandmarkCountHeuristic(const options::Options &opts)
    : Heuristic(opts),
      use_preferred_operators(opts.get<bool>("pref")),
      conditional_effects_supported(
          opts.get<shared_ptr<LandmarkFactory>>("lm_factory")->supports_conditional_effects()),
      admissible(opts.get<bool>("admissible")),
      dead_ends_reliable(
          admissible ||
          (!task_properties::has_axioms(task_proxy) &&
           (!task_properties::has_conditional_effects(task_proxy) || conditional_effects_supported))),
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

    if (admissible) {
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

    if (admissible) {
        if (opts.get<bool>("optimal")) {
            lm_cost_assignment = utils::make_unique_ptr<LandmarkEfficientOptimalSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph, opts.get<lp::LPSolverType>("lpsolver"));
        } else {
            lm_cost_assignment = utils::make_unique_ptr<LandmarkUniformSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph, opts.get<bool>("alm"));
        }
    } else {
        lm_cost_assignment = nullptr;
        compute_landmark_costs();
    }

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main task in cases
           where it's compatible. See issue564. */
        successor_generator = utils::make_unique_ptr<successor_generator::SuccessorGenerator>(task_proxy);
    }
}

int LandmarkCountHeuristic::get_min_cost_of_achievers(const set<int> &achievers,
                                                      const TaskProxy &task_proxy) {
    int min_cost = numeric_limits<int>::max();
    for (int id : achievers) {
        OperatorProxy op = get_operator_or_axiom(task_proxy, id);
        min_cost = min(min_cost, op.get_cost());
    }
    return min_cost;
}

void LandmarkCountHeuristic::compute_landmark_costs() {
    /*
       This function runs under the assumption that landmark node IDs go
       from 0 to the number of landmarks - 1, therefore the entry in
       *min_first_achiever_costs* and *min_possible_achiever_costs*
       at index i corresponds to the entry for the landmark node with ID i.
    */

    /*
       For derived landmarks, we overapproximate that all operators are achievers.
       Since we do not want to explicitly store all operators in the achiever
       vector, we instead just compute the minimum cost over all operators and
       use this cost for all derived landmarks.
    */
    int min_operator_cost = task_properties::get_min_operator_cost(task_proxy);
    min_first_achiever_costs.reserve(lgraph->get_num_landmarks());
    min_possible_achiever_costs.reserve(lgraph->get_num_landmarks());
    for (auto &node : lgraph->get_nodes()) {
        if (node->get_landmark().is_derived) {
            min_first_achiever_costs.push_back(min_operator_cost);
            min_possible_achiever_costs.push_back(min_operator_cost);
        } else {
            int min_first_achiever_cost = get_min_cost_of_achievers(
                node->get_landmark().first_achievers, task_proxy);
            min_first_achiever_costs.push_back(min_first_achiever_cost);
            int min_possible_achiever_cost = get_min_cost_of_achievers(
                node->get_landmark().possible_achievers, task_proxy);
            min_possible_achiever_costs.push_back(min_possible_achiever_cost);
        }
    }
}

int LandmarkCountHeuristic::get_heuristic_value(const State &ancestor_state) {
    double epsilon = 0.01;

    // Need explicit test to see if state is a goal state. The landmark
    // heuristic may compute h != 0 for a goal state if landmarks are
    // achieved before their parents in the landmarks graph (because
    // they do not get counted as reached in that case). However, we
    // must return 0 for a goal state.

    lm_status_manager->update_lm_status(ancestor_state);

    if (admissible) {
        double h_val = lm_cost_assignment->cost_sharing_h_value(
            *lm_status_manager);
        if (h_val == numeric_limits<double>::max()) {
            return DEAD_END;
        } else {
            return static_cast<int>(ceil(h_val - epsilon));
        }
    } else {
        int h = 0;
        for (int id = 0; id < lgraph->get_num_landmarks(); ++id) {
            landmark_status status = lm_status_manager->get_landmark_status(id);
            if (status == lm_not_reached) {
                if (min_first_achiever_costs[id] == numeric_limits<int>::max())
                    return DEAD_END;
                h += min_first_achiever_costs[id];
            } else if (status == lm_needed_again) {
                if (min_possible_achiever_costs[id] == numeric_limits<int>::max())
                    return DEAD_END;
                h += min_possible_achiever_costs[id];
            }
        }
        return h;
    }
}

int LandmarkCountHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);

    if (task_properties::is_goal_state(task_proxy, state))
        return 0;

    int h = get_heuristic_value(ancestor_state);

    if (use_preferred_operators) {
        BitsetView reached_lms =
            lm_status_manager->get_reached_landmarks(ancestor_state);
        generate_helpful_actions(state, reached_lms);
    }

    return h;
}

bool LandmarkCountHeuristic::check_node_orders_disobeyed(
    const LandmarkNode &node, const BitsetView &reached) const {
    for (const auto &parent : node.parents) {
        if (!reached.test(parent.first->get_id())) {
            return true;
        }
    }
    return false;
}

bool LandmarkCountHeuristic::generate_helpful_actions(
    const State &state, const BitsetView &reached) {
    /*
      Find actions that achieve new landmark leaves. If no such action exist,
      return false. If a simple landmark can be achieved, return only operators
      that achieve simple landmarks, else return operators that achieve
      disjunctive landmarks.
    */
    assert(successor_generator);
    vector<OperatorID> applicable_operators;
    successor_generator->generate_applicable_ops(state, applicable_operators);
    vector<OperatorID> ha_simple;
    vector<OperatorID> ha_disj;

    for (OperatorID op_id : applicable_operators) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        EffectsProxy effects = op.get_effects();
        for (EffectProxy effect : effects) {
            if (!does_fire(effect, state))
                continue;
            FactProxy fact_proxy = effect.get_fact();
            LandmarkNode *lm_node = lgraph->get_node(fact_proxy.get_pair());
            if (lm_node && landmark_is_interesting(state, reached, *lm_node)) {
                if (lm_node->get_landmark().disjunctive) {
                    ha_disj.push_back(op_id);
                } else {
                    ha_simple.push_back(op_id);
                }
            }
        }
    }
    if (ha_disj.empty() && ha_simple.empty())
        return false;

    OperatorsProxy operators = task_proxy.get_operators();
    if (ha_simple.empty()) {
        for (OperatorID op_id : ha_disj) {
            set_preferred(operators[op_id]);
        }
    } else {
        for (OperatorID op_id : ha_simple) {
            set_preferred(operators[op_id]);
        }
    }
    return true;
}

bool LandmarkCountHeuristic::landmark_is_interesting(
    const State &state, const BitsetView &reached,
    LandmarkNode &lm_node) const {
    /*
      A landmark is interesting if it hasn't been reached before and
      its parents have all been reached, or if all landmarks have been
      reached before, the LM is a goal, and it's not true at moment.
    */

    bool all_reached = true;
    for (int i = 0; i < reached.size(); ++i) {
        if (!reached.test(i)) {
            all_reached = false;
            break;
        }
    }

    if (all_reached) {
        const Landmark &landmark = lm_node.get_landmark();
        return landmark.is_true_in_goal && !landmark.is_true_in_state(state);
    } else {
        if (reached.test(lm_node.get_id()))
            return false;
        else
            return !check_node_orders_disobeyed(lm_node, reached);
    }
}

void LandmarkCountHeuristic::notify_initial_state(const State &initial_state) {
    lm_status_manager->process_initial_state(initial_state, log);
}

void LandmarkCountHeuristic::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    lm_status_manager->process_state_transition(parent_state, op_id, state);
    if (cache_evaluator_values) {
        /* TODO:  It may be more efficient to check that the reached landmark
           set has actually changed and only then mark the h value as dirty. */
        heuristic_cache[state].dirty = true;
    }
}

bool LandmarkCountHeuristic::dead_ends_are_reliable() const {
    return dead_ends_reliable;
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Landmark-count heuristic",
        "For the inadmissible variant see the papers" +
        utils::format_conference_reference(
            {"Silvia Richter", "Malte Helmert", "Matthias Westphal"},
            "Landmarks Revisited",
            "https://ai.dmi.unibas.ch/papers/richter-et-al-aaai2008.pdf",
            "Proceedings of the 23rd AAAI Conference on Artificial "
            "Intelligence (AAAI 2008)",
            "975-982",
            "AAAI Press",
            "2008") +
        "and" +
        utils::format_journal_reference(
            {"Silvia Richter", "Matthias Westphal"},
            "The LAMA Planner: Guiding Cost-Based Anytime Planning with Landmarks",
            "http://www.aaai.org/Papers/JAIR/Vol39/JAIR-3903.pdf",
            "Journal of Artificial Intelligence Research",
            "39",
            "127-177",
            "2010") +
        "For the admissible variant see the papers" +
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

    parser.document_note(
        "Optimal search",
        "When using landmarks for optimal search (``admissible=true``), "
        "you probably also want to add this heuristic as a lazy_evaluator "
        "in the A* algorithm to improve heuristic estimates.");
    parser.document_note(
        "Note",
        "To use ``optimal=true``, you must build the planner with LP support. "
        "See LPBuildInstructions.");
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

    parser.document_language_support("action costs",
                                     "supported");
    parser.document_language_support("conditional_effects",
                                     "supported if the LandmarkFactory supports "
                                     "them; otherwise ignored with "
                                     "``admissible=false`` and not allowed with "
                                     "``admissible=true``");
    parser.document_language_support("axioms",
                                     "ignored with ``admissible=false``; not "
                                     "allowed with ``admissible=true``");
    parser.document_property("admissible",
                             "yes if ``admissible=true``");
    // TODO: this was "yes with admissible=true and optimal cost
    // partitioning; otherwise no" before.
    parser.document_property("consistent",
                             "complicated; needs further thought");
    parser.document_property("safe",
                             "yes except on tasks with axioms or on tasks with "
                             "conditional effects when using a LandmarkFactory "
                             "not supporting them");
    parser.document_property("preferred operators",
                             "yes (if enabled; see ``pref`` option)");

    parser.add_option<shared_ptr<LandmarkFactory>>(
        "lm_factory",
        "the set of landmarks to use for this heuristic. "
        "The set of landmarks can be specified here, "
        "or predefined (see LandmarkFactory).");
    parser.add_option<bool>("admissible", "get admissible estimate", "false");
    parser.add_option<bool>(
        "optimal",
        "use optimal (LP-based) cost sharing "
        "(only makes sense with ``admissible=true``)", "false");
    parser.add_option<bool>("pref", "identify preferred operators "
                                    "(see OptionCaveats#Using_preferred_operators_"
                                    "with_the_lmcount_heuristic)", "false");
    parser.add_option<bool>("alm", "use action landmarks", "true");
    lp::add_lp_solver_option_to_parser(parser);
    Heuristic::add_options_to_parser(parser);

    parser.document_note("Note on performance for satisficing planning",
                         "The cost of a landmark is based on the cost of the "
                         "operators that achieve it. For satisficing search "
                         "this can be counterproductive since it is often "
                         "better to focus on distance from goal "
                         "(i.e. length of the plan) rather than cost."
                         "In experiments we achieved the best performance using"
                         "the option 'transform=adapt_costs(one)' to enforce "
                         "unit costs.");
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkCountHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("lmcount", _parse);
}
