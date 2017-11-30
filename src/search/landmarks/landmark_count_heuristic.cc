#include "landmark_count_heuristic.h"

#include "landmark_cost_assignment.h"
#include "landmark_factory.h"
#include "landmark_status_manager.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../lp/lp_solver.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cmath>
#include <limits>
#include <unordered_map>

using namespace std;
using utils::ExitCode;

namespace landmarks {
static Options get_exploration_options(
    const shared_ptr<AbstractTask> &task, bool cache_estimates) {
    Options exploration_opts;
    exploration_opts.set<shared_ptr<AbstractTask>>("transform", task);
    exploration_opts.set<bool>("cache_estimates", cache_estimates);
    return exploration_opts;
}

LandmarkCountHeuristic::LandmarkCountHeuristic(const options::Options &opts)
    : Heuristic(opts),
      exploration(get_exploration_options(task, cache_h_values)),
      use_preferred_operators(opts.get<bool>("pref")),
      ff_search_disjunctive_lms(false),
      conditional_effects_supported(
          opts.get<LandmarkFactory *>("lm_factory")->supports_conditional_effects()),
      admissible(opts.get<bool>("admissible")),
      dead_ends_reliable(
          admissible ||
          (!task_properties::has_axioms(task_proxy) &&
           (!task_properties::has_conditional_effects(task_proxy) || conditional_effects_supported))),
      successor_generator(nullptr) {
    cout << "Initializing landmarks count heuristic..." << endl;
    LandmarkFactory *lm_graph_factory = opts.get<LandmarkFactory *>("lm_factory");
    lgraph = lm_graph_factory->compute_lm_graph(task, exploration);
    bool reasonable_orders = lm_graph_factory->use_reasonable_orders();
    lm_status_manager = utils::make_unique_ptr<LandmarkStatusManager>(*lgraph);

    if (admissible) {
        if (reasonable_orders) {
            cerr << "Reasonable orderings should not be used for admissible heuristics" << endl;
            utils::exit_with(ExitCode::INPUT_ERROR);
        } else if (task_properties::has_axioms(task_proxy)) {
            cerr << "cost partitioning does not support axioms" << endl;
            utils::exit_with(ExitCode::UNSUPPORTED);
        } else if (task_properties::has_conditional_effects(task_proxy) &&
                   !conditional_effects_supported) {
            cerr << "conditional effects not supported by the landmark generation method" << endl;
            utils::exit_with(ExitCode::UNSUPPORTED);
        }
        if (opts.get<bool>("optimal")) {
            lm_cost_assignment = utils::make_unique_ptr<LandmarkEfficientOptimalSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph,
                static_cast<lp::LPSolverType>(opts.get_enum("lpsolver")));
        } else {
            lm_cost_assignment = utils::make_unique_ptr<LandmarkUniformSharedCostAssignment>(
                task_properties::get_operator_costs(task_proxy),
                *lgraph, opts.get<bool>("alm"));
        }
    } else {
        lm_cost_assignment = nullptr;
    }

    if (use_preferred_operators) {
        /* Ideally, we should reuse the successor generator of the main task in cases
           where it's compatible. See issue564. */
        successor_generator = utils::make_unique_ptr<successor_generator::SuccessorGenerator>(task_proxy);
    }
}

LandmarkCountHeuristic::~LandmarkCountHeuristic() {
}

void LandmarkCountHeuristic::set_exploration_goals(const GlobalState &global_state) {
    // Set additional goals for FF exploration
    LandmarkSet reached_landmarks = convert_to_landmark_set(
        lm_status_manager->get_reached_landmarks(global_state));
    vector<FactPair> lm_leaves = collect_lm_leaves(
        ff_search_disjunctive_lms, reached_landmarks);
    exploration.set_additional_goals(lm_leaves);
}

int LandmarkCountHeuristic::get_heuristic_value(const GlobalState &global_state) {
    double epsilon = 0.01;

    // Need explicit test to see if state is a goal state. The landmark
    // heuristic may compute h != 0 for a goal state if landmarks are
    // achieved before their parents in the landmarks graph (because
    // they do not get counted as reached in that case). However, we
    // must return 0 for a goal state.

    bool dead_end = lm_status_manager->update_lm_status(global_state);

    if (dead_end) {
        return DEAD_END;
    }

    int h = -1;

    if (admissible) {
        double h_val = lm_cost_assignment->cost_sharing_h_value();
        h = static_cast<int>(ceil(h_val - epsilon));
    } else {
        lgraph->count_costs();

        int total_cost = lgraph->cost_of_landmarks();
        int reached_cost = lgraph->get_reached_cost();
        int needed_cost = lgraph->get_needed_cost();

        h = total_cost - reached_cost + needed_cost;
    }

    assert(h >= 0);
    return h;
}

int LandmarkCountHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);

    if (task_properties::is_goal_state(task_proxy, state))
        return 0;

    int h = get_heuristic_value(global_state);

    // no (need for) helpful actions, return
    if (!use_preferred_operators) {
        return h;
    }

    // Try generating helpful actions (those that lead to new leaf LM in the
    // next step). If all LMs have been reached before or no new ones can be
    // reached within next step, helpful actions are those occuring in a plan
    // to achieve one of the LM leaves.

    LandmarkSet reached_lms = convert_to_landmark_set(
        lm_status_manager->get_reached_landmarks(global_state));

    int num_reached = reached_lms.size();
    if (num_reached == lgraph->number_of_landmarks() ||
        !generate_helpful_actions(state, reached_lms)) {
        set_exploration_goals(global_state);

        // Use FF to plan to a landmark leaf.
        vector<FactPair> leaves = collect_lm_leaves(
            ff_search_disjunctive_lms, reached_lms);
        if (!exploration.plan_for_disj(leaves, state)) {
            exploration.exported_op_ids.clear();
            return DEAD_END;
        }
        OperatorsProxy operators = task_proxy.get_operators();
        for (int exported_op_id : exploration.exported_op_ids) {
            set_preferred(operators[exported_op_id]);
        }
        exploration.exported_op_ids.clear();
    }

    return h;
}

vector<FactPair> LandmarkCountHeuristic::collect_lm_leaves(
    bool disjunctive_lms, const LandmarkSet &reached_lms) {
    vector<FactPair> leaves;
    for (const LandmarkNode *node_p : lgraph->get_nodes()) {
        if (!disjunctive_lms && node_p->disjunctive)
            continue;

        if (!reached_lms.count(node_p) &&
            !check_node_orders_disobeyed(*node_p, reached_lms)) {
            leaves.insert(
                leaves.end(), node_p->facts.begin(), node_p->facts.end());
        }
    }
    return leaves;
}

bool LandmarkCountHeuristic::check_node_orders_disobeyed(const LandmarkNode &node,
                                                         const LandmarkSet &reached) const {
    for (const auto &parent : node.parents) {
        if (reached.count(parent.first) == 0) {
            return true;
        }
    }
    return false;
}

bool LandmarkCountHeuristic::generate_helpful_actions(const State &state,
                                                      const LandmarkSet &reached) {
    /* Find actions that achieve new landmark leaves. If no such action exist,
     return false. If a simple landmark can be achieved, return only operators
     that achieve simple landmarks, else return operators that achieve
     disjunctive landmarks */
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
            LandmarkNode *lm_p = lgraph->get_landmark(fact_proxy.get_pair());
            if (lm_p != 0 && landmark_is_interesting(state, reached, *lm_p)) {
                if (lm_p->disjunctive) {
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
    const State &state, const LandmarkSet &reached, LandmarkNode &lm) const {
    /* A landmark is interesting if it hasn't been reached before and
     its parents have all been reached, or if all landmarks have been
     reached before, the LM is a goal, and it's not true at moment */

    int num_reached = reached.size();
    if (num_reached != lgraph->number_of_landmarks()) {
        if (reached.find(&lm) != reached.end())
            return false;
        else
            return !check_node_orders_disobeyed(lm, reached);
    }
    return lm.is_goal() && !lm.is_true_in_state(state);
}

void LandmarkCountHeuristic::notify_initial_state(const GlobalState &initial_state) {
    lm_status_manager->set_landmarks_for_initial_state(initial_state);
}

bool LandmarkCountHeuristic::notify_state_transition(
    const GlobalState &parent_state, OperatorID op_id,
    const GlobalState &state) {
    lm_status_manager->update_reached_lms(parent_state, op_id, state);
    /* TODO: The return value "true" signals that the LM set of this state
             has changed and the h value should be recomputed. It's not
             wrong to always return true, but it may be more efficient to
             check that the LM set has actually changed. */
    if (cache_h_values) {
        heuristic_cache[state].dirty = true;
    }
    return true;
}

bool LandmarkCountHeuristic::dead_ends_are_reliable() const {
    return dead_ends_reliable;
}

// This function exists purely so we don't have to change all the
// functions in this class that use LandmarkSets for the reached LMs
// (HACK).
LandmarkSet LandmarkCountHeuristic::convert_to_landmark_set(
    const vector<bool> &landmark_vector) {
    LandmarkSet landmark_set;
    for (size_t i = 0; i < landmark_vector.size(); ++i)
        if (landmark_vector[i])
            landmark_set.insert(lgraph->get_lm_for_index(i));
    return landmark_set;
}


static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis("Landmark-count heuristic",
                             "See also Synergy");
    parser.document_note(
        "Note",
        "to use ``optimal=true``, you must build the planner with LP support. "
        "See LPBuildInstructions.");
    parser.document_note(
        "Optimal search",
        "when using landmarks for optimal search (``admissible=true``), "
        "you probably also want to enable the mpd option of the A* algorithm "
        "to improve heuristic estimates");
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

    parser.add_option<LandmarkFactory *>(
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
    Options opts = parser.parse();

    if (parser.dry_run())
        return nullptr;
    else
        return new LandmarkCountHeuristic(opts);
}

static Plugin<Heuristic> _plugin(
    "lmcount", _parse);
}
