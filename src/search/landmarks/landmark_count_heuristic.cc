#include "landmark_count_heuristic.h"

#include "../plugin.h"
#include "../successor_generator.h"

#include <cmath>
#include <ext/hash_map>
#include <limits>

using namespace std;
using namespace __gnu_cxx;

LandmarkCountHeuristic::LandmarkCountHeuristic(const Options &opts)
    : Heuristic(opts),
      lgraph(*opts.get<LandmarkGraph *>("lm_graph")),
      exploration(lgraph.get_exploration()),
      lm_status_manager(lgraph) {
    cout << "Initializing landmarks count heuristic..." << endl;
    use_preferred_operators = opts.get<bool>("pref");
    lookahead = numeric_limits<int>::max();
    // When generating preferred operators, we plan towards
    // non-disjunctive landmarks only
    ff_search_disjunctive_lms = false;

    if (opts.get<bool>("admissible")) {
        use_cost_sharing = true;
        if (lgraph.is_using_reasonable_orderings()) {
            cerr << "Reasonable orderings should not be used for admissible heuristics" << endl;
            exit_with(EXIT_INPUT_ERROR);
        }
        if (!g_axioms.empty()) {
            cerr << "cost partitioning does not support axioms" << endl;
            exit_with(EXIT_UNSUPPORTED);
        }
        if (opts.get<bool>("optimal")) {
#ifdef USE_LP
            lm_cost_assignment = new LandmarkEfficientOptimalSharedCostAssignment(lgraph,
                                                                                  OperatorCost(opts.get_enum("cost_type")));
#else
            cerr << "You must build the planner with the USE_LP symbol defined." << endl
                 << "If you already did, try \"make clean\" before rebuilding with USE_LP=1." << endl;
            exit_with(EXIT_INPUT_ERROR);
#endif
        } else {
            lm_cost_assignment = new LandmarkUniformSharedCostAssignment(lgraph, opts.get<bool>("alm"),
                                                                         OperatorCost(opts.get_enum("cost_type")));
        }
    } else {
        use_cost_sharing = false;
        lm_cost_assignment = 0;
    }

    lm_status_manager.set_landmarks_for_initial_state();
}

void LandmarkCountHeuristic::set_exploration_goals(const State &state) {
    assert(exploration != 0);
    // Set additional goals for FF exploration
    vector<pair<int, int> > lm_leaves;
    LandmarkSet result;
    const vector<bool> &reached_lms_v = lm_status_manager.get_reached_landmarks(state);
    convert_lms(result, reached_lms_v);
    collect_lm_leaves(ff_search_disjunctive_lms, result, lm_leaves);
    exploration->set_additional_goals(lm_leaves);
}

int LandmarkCountHeuristic::get_heuristic_value(const State &state) {
    double epsilon = 0.01;

    // Need explicit test to see if state is a goal state. The landmark
    // heuristic may compute h != 0 for a goal state if landmarks are
    // achieved before their parents in the landmarks graph (because
    // they do not get counted as reached in that case). However, we
    // must return 0 for a goal state.

    bool dead_end = lm_status_manager.update_lm_status(state);

    if (dead_end) {
        return DEAD_END;
    }

    int h = -1;

    if (use_cost_sharing) {
        double h_val = lm_cost_assignment->cost_sharing_h_value();
        h = ceil(h_val - epsilon);
    } else {
        lgraph.count_costs();

        int total_cost = lgraph.cost_of_landmarks();
        int reached_cost = lgraph.get_reached_cost();
        int needed_cost = lgraph.get_needed_cost();

        h = total_cost - reached_cost + needed_cost;
    }

    // Two plausibility tests in debug mode.
    assert(h >= 0);
    if (h == 0 && g_min_action_cost > 0)
        assert(test_goal(state));

    return h;
}

int LandmarkCountHeuristic::compute_heuristic(const State &state) {
    bool goal_reached = test_goal(state);
    if (goal_reached)
        return 0;

    int h = get_heuristic_value(state);

    // no (need for) helpful actions, return
    if (!use_preferred_operators) {
        return h;
    }

    // Try generating helpful actions (those that lead to new leaf LM in the
    // next step). If all LMs have been reached before or no new ones can be
    // reached within next step, helpful actions are those occuring in a plan
    // to achieve one of the LM leaves.

    LandmarkSet reached_lms;
    vector<bool> &reached_lms_v = lm_status_manager.get_reached_landmarks(state);
    convert_lms(reached_lms, reached_lms_v);

    if (reached_lms.size() == lgraph.number_of_landmarks()
        || !generate_helpful_actions(state, reached_lms)) {
        assert(exploration != NULL);
        set_exploration_goals(state);

        // Use FF to plan to a landmark leaf
        vector<pair<int, int> > leaves;
        collect_lm_leaves(ff_search_disjunctive_lms, reached_lms, leaves);
        if (!exploration->plan_for_disj(leaves, state)) {
            exploration->exported_ops.clear();
            return DEAD_END;
        }
        for (int i = 0; i < exploration->exported_ops.size(); i++) {
            set_preferred(exploration->exported_ops[i]);
        }
        exploration->exported_ops.clear();
    }

    return h;
}

void LandmarkCountHeuristic::collect_lm_leaves(bool disjunctive_lms,
                                               LandmarkSet &reached_lms, vector<pair<int, int> > &leaves) {
    set<LandmarkNode *>::const_iterator it;
    for (it = lgraph.get_nodes().begin(); it != lgraph.get_nodes().end(); it++) {
        LandmarkNode *node_p = *it;

        if (!disjunctive_lms && node_p->disjunctive)
            continue;

        if (reached_lms.find(node_p) == reached_lms.end()
            && !check_node_orders_disobeyed(*node_p, reached_lms)) {
            for (int i = 0; i < node_p->vars.size(); i++) {
                pair<int, int> node_prop = make_pair(node_p->vars[i],
                                                     node_p->vals[i]);
                leaves.push_back(node_prop);
            }
        }
    }
}

bool LandmarkCountHeuristic::check_node_orders_disobeyed(LandmarkNode &node,
                                                         const LandmarkSet &reached) const {
    const hash_map<LandmarkNode *, edge_type, hash_pointer> &parents =
        node.parents;
    for (hash_map<LandmarkNode *, edge_type, hash_pointer>::const_iterator
         parent_it = parents.begin(); parent_it != parents.end(); parent_it++) {
        LandmarkNode &parent = *(parent_it->first);
        if (reached.find(&parent) == reached.end()) {
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
    vector<const Operator *> all_operators;
    g_successor_generator->generate_applicable_ops(state, all_operators);
    vector<const Operator *> ha_simple;
    vector<const Operator *> ha_disj;

    for (int i = 0; i < all_operators.size(); i++) {
        const vector<PrePost> &prepost = all_operators[i]->get_pre_post();
        for (int j = 0; j < prepost.size(); j++) {
            if (!prepost[j].does_fire(state))
                continue;
            const pair<int, int> varval = make_pair(prepost[j].var,
                                                    prepost[j].post);
            LandmarkNode *lm_p = lgraph.get_landmark(varval);
            if (lm_p != 0 && landmark_is_interesting(state, reached, *lm_p)) {
                if (lm_p->disjunctive) {
                    ha_disj.push_back(all_operators[i]);
                } else
                    ha_simple.push_back(all_operators[i]);
            }
        }
    }
    if (ha_disj.empty() && ha_simple.empty())
        return false;

    if (ha_simple.empty()) {
        for (int i = 0; i < ha_disj.size(); i++) {
            set_preferred(ha_disj[i]);
        }
    } else {
        for (int i = 0; i < ha_simple.size(); i++) {
            set_preferred(ha_simple[i]);
        }
    }
    return true;
}

bool LandmarkCountHeuristic::landmark_is_interesting(const State &s,
                                                     const LandmarkSet &reached, LandmarkNode &lm) const {
    /* A landmark is interesting if it hasn't been reached before and
     its parents have all been reached, or if all landmarks have been
     reached before, the LM is a goal, and it's not true at moment */

    if (lgraph.number_of_landmarks() != reached.size()) {
        if (reached.find(&lm) != reached.end())
            return false;
        else
            return !check_node_orders_disobeyed(lm, reached);
    }
    return lm.is_goal() && !lm.is_true_in_state(s);
}

bool LandmarkCountHeuristic::reach_state(const State &parent_state,
                                         const Operator &op, const State &state) {
    lm_status_manager.update_reached_lms(parent_state, op, state);
    /* TODO: The return value "true" signals that the LM set of this state
             has changed and the h value should be recomputed. It's not
             wrong to always return true, but it may be more efficient to
             check that the LM set has actually changed. */
    return true;
}

void LandmarkCountHeuristic::reset() {
    lm_status_manager.clear_reached();
    lm_status_manager.set_landmarks_for_initial_state();
}

void LandmarkCountHeuristic::convert_lms(LandmarkSet &lms_set,
                                         const vector<bool> &lms_vec) {
    // This function exists purely so we don't have to change all the
    // functions in this class that use LandmarkSets for the reached LMs
    // (HACK).

    for (int i = 0; i < lms_vec.size(); i++)
        if (lms_vec[i])
            lms_set.insert(lgraph.get_lm_for_index(i));
}


static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.add_option<LandmarkGraph *>("lm_graph");
    parser.add_option<bool>("admissible", false, "get admissible estimate");
    parser.add_option<bool>("optimal", false, "optimal cost sharing");
    parser.add_option<bool>("pref", false, "identify preferred operators");
    parser.add_option<bool>("alm", true, "use action landmarks");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();

    if (!parser.dry_run() && opts.get<LandmarkGraph *>("lm_graph") == 0)
        parser.error("landmark graph could not be constructed");

    if (parser.dry_run())
        return 0;
    else
        return new LandmarkCountHeuristic(opts);
}

static Plugin<ScalarEvaluator> _plugin(
    "lmcount", _parse);
