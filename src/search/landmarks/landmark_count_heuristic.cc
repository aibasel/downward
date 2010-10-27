#include <climits>
#include <math.h>

#include "landmark_count_heuristic.h"
#include "landmarks_graph_rpg_sasp.h"
#include "landmarks_graph_zhu_givan.h"
#include "landmarks_graph_rpg_exhaust.h"
#include "landmarks_graph_rpg_search.h"

#include "../globals.h"
#include "../operator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../search_engine.h"
#include "../successor_generator.h"
#include "../timer.h"

LandmarksGraph *g_lgraph; // global to be accessible by state


static ScalarEvaluatorPlugin landmark_count_heuristic_plugin(
    "lmcount", LandmarkCountHeuristic::create);


static void init_lm_graph(Exploration *exploration, int landmarks_type) {
    switch (landmarks_type) {
    case LandmarkCountHeuristic::rpg_sasp:
        g_lgraph = new LandmarksGraphNew(exploration);
        break;
    case LandmarkCountHeuristic::zhu_givan:
        g_lgraph = new LandmarksGraphZhuGivan(exploration); // Note: remove exploration here?
        break;
    case LandmarkCountHeuristic::exhaust:
        g_lgraph = new LandmarksGraphExhaust(exploration);
        break;
    case LandmarkCountHeuristic::search:
        g_lgraph = new LandmarksGraphRpgSearch(exploration);
        break;
    default:
        assert(false); // unknown landmarks_type
    }
}

static LandmarksGraph *build_landmarks_graph(Exploration *exploration, bool admissible,
                                             int landmarks_type = LandmarkCountHeuristic::rpg_sasp) {
    bool reasonable_orders = true; // option to use/not use reasonable orderings
    bool disjunctive_lms = true; // option to discard/not discard disj. landmarks before search
    if (admissible) {
        reasonable_orders = false;
        disjunctive_lms = false;
    }
    if (g_lgraph != NULL) // in case of iterated search LM graph has already been constructed
        return g_lgraph;
    Timer lm_generation_timer;
    init_lm_graph(exploration, landmarks_type);
    g_lgraph->read_external_inconsistencies();
    if (reasonable_orders) {
        g_lgraph->use_reasonable_orders();
    }
    g_lgraph->generate();
    cout << "Landmarks generation time: " << lm_generation_timer << endl;
    if (g_lgraph->number_of_landmarks() == 0)
        cout << "Warning! No landmarks found. Task unsolvable?" << endl;
    cout << "Discovered " << g_lgraph->number_of_landmarks()
         << " landmarks, of which " << g_lgraph->number_of_disj_landmarks()
         << " are disjunctive" << endl << "          "
         << g_lgraph->number_of_edges() << " edges\n";
    if (!disjunctive_lms)
        g_lgraph->discard_disjunctive_landmarks();
    //g_lgraph->dump();
    return g_lgraph;
}

LandmarkCountHeuristic::LandmarkCountHeuristic(bool preferred_ops,
                                               bool admissible, bool optimal, int landmarks_type)
    : exploration(new Exploration),
      lgraph(*build_landmarks_graph(exploration, admissible, landmarks_type)),
      lm_status_manager(lgraph) {
    cout << "Initializing landmarks count heuristic..." << endl;
    use_preferred_operators = preferred_ops;
    lookahead = INT_MAX;
    // When generating preferred operators, we plan towards
    // non-disjunctive landmarks only
    ff_search_disjunctive_lms = false;
    // Turn goal into hash set
    goal.resize(g_goal.size());
    for (int i = 0; i < g_goal.size(); i++)
        goal.insert(make_pair(g_goal[i].first, g_goal[i].second));

    if (optimal) {
#ifdef USE_LP
        lm_cost_assignment = new LandmarkOptimalSharedCostAssignment(lgraph, true);
#else
        cerr << "You must build the planner with the USE_LP symbol defined." << endl
             << "If you already did, try \"make clean\" before rebuilding with USE_LP=1." << endl;
        exit(1);
#endif
    } else {
        lm_cost_assignment = new LandmarkUniformSharedCostAssignment(lgraph, true);
    }


    if (admissible) {
        use_shared_cost = true;
        use_dynamic_cost_sharing = true;
        use_action_landmark_count = true;
    } else {
        use_shared_cost = false;
        use_dynamic_cost_sharing = false;
        use_action_landmark_count = false;
    }
    lm_status_manager.set_landmarks_for_initial_state(*g_initial_state);
}

void LandmarkCountHeuristic::set_exploration_goals(const State &state) {
    assert(exploration != 0);
    // Set additional goals for FF exploration
    vector<pair<int, int> > lm_leaves;
    LandmarkSet &result = lm_status_manager.get_reached_landmarks(state);
    collect_lm_leaves(ff_search_disjunctive_lms, result, lm_leaves);
    exploration->set_additional_goals(lm_leaves);
}

int LandmarkCountHeuristic::get_heuristic_value(const State &state) {
    // Need explicit test to see if state is a goal state. The landmark
    // heuristic may compute h != 0 for a goal state if landmarks are
    // achieved before their parents in the landmarks graph (because
    // they do not get counted as reached in that case). However, we
    // must return 0 for a goal state.

    bool goal_reached = true;
    for (int i = 0; i < g_goal.size(); i++)
        if (state[g_goal[i].first] != g_goal[i].second)
            goal_reached = false;

    if (goal_reached) {
        return 0;
    }

    //lm_status_manager.update_lm_status(state);
    bool dead_end = lm_status_manager.update_lm_status(state);
    if (dead_end) {
        return DEAD_END;
    }

    if (use_dynamic_cost_sharing) {
        lm_cost_assignment->assign_costs();
        lgraph.count_shared_costs();
    }
    lgraph.count_costs();

    //cout << "After cost sharing ---------------------------------------------------- " << endl;
    //lgraph.dump();

    double total_cost = 0;
    double reached_cost = 0;
    double needed_cost = 0;
    double additional_cost = 0;
    double epsilon = 0.01;

    if (use_shared_cost) {
        total_cost = lgraph.shared_cost_of_landmarks();
        if (use_action_landmark_count) {
            additional_cost = lgraph.get_unused_action_landmark_cost();
            reached_cost =
                lgraph.get_not_unused_alm_effect_reached_shared_cost();
            needed_cost = lgraph.get_not_unused_alm_effect_needed_shared_cost();
        } else {
            reached_cost = lgraph.get_reached_shared_cost();
            needed_cost = lgraph.get_needed_shared_cost();
        }
    } else {
        total_cost = (double)lgraph.cost_of_landmarks();
        reached_cost = (double)lgraph.get_reached_cost();
        needed_cost = (double)lgraph.get_needed_cost();
    }

    int h = ceil(total_cost - reached_cost + needed_cost + additional_cost
                 - epsilon);
    /*
     cout << "h = " << total_cost << " - " << reached_cost << " + " << needed_cost
          << " + " << additional_cost << " = " << h << endl;
    */

    assert(-1 * epsilon <= needed_cost);
    assert(reached_cost - needed_cost >= -1 * epsilon);
    assert(h >= 0);

#ifndef NDEBUG
    // For debugging purposes, check whether heuristic is 0 even though
    // goal is not reached. This should never happen unless action costs
    // are used where some actions have cost 0.
    if (h == 0 && !goal_reached) {
        assert(g_use_metric);
        int max = 0;
        //cout << "WARNING! Landmark heuristic is 0, but goal not reached" << endl;
        for (int i = 0; i < g_goal.size(); i++) {
            if (state[g_goal[i].first] != g_goal[i].second) {
                //cout << "missing goal prop " << g_variable_name[g_goal[i].first] << " : "
                //<< g_goal[i].second << endl;
                LandmarkNode *node_p = lgraph.landmark_reached(g_goal[i]);
                assert(node_p != NULL);
                int cost = node_p->min_cost;

                if (cost > max)
                    max = cost;
            }
        }
        assert(max == 0);
    }
#endif

    return h;
}

int LandmarkCountHeuristic::compute_heuristic(const State &state) {
    int h = get_heuristic_value(state);

    if (!use_preferred_operators || h == 0) { // no (need for) helpful actions, return
        return h;
    }

    // Try generating helpful actions (those that lead to new leaf LM in the
    // next step). If all LMs have been reached before or no new ones can be
    // reached within next step, helpful actions are those occuring in a plan
    // to achieve one of the LM leaves.

    LandmarkSet &reached_lms = lm_status_manager.get_reached_landmarks(state);
    const int reached_lms_cost = lgraph.get_reached_cost();

    if (reached_lms_cost == lgraph.cost_of_landmarks()
        || !generate_helpful_actions(state, reached_lms)) {
        assert(exploration != NULL);
        set_exploration_goals(state);
        // Use FF to plan to a landmark leaf
        int dead_end = ff_search_lm_leaves(ff_search_disjunctive_lms, state,
                                           reached_lms);
        if (dead_end) {
            assert(dead_end == DEAD_END);
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

int LandmarkCountHeuristic::ff_search_lm_leaves(bool disjunctive_lms,
                                                const State &state, LandmarkSet &reached_lms) {
    vector<pair<int, int> > leaves;
    collect_lm_leaves(disjunctive_lms, reached_lms, leaves);
    if (exploration->plan_for_disj(leaves, state) == DEAD_END) {
        return DEAD_END;
    } else
        return 0;
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
            LandmarkNode *lm_p = lgraph.landmark_reached(varval);
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

    return true;
}

void LandmarkCountHeuristic::reset() {
    lm_status_manager.clear_reached();
    lm_status_manager.set_landmarks_for_initial_state(*g_initial_state);
}

ScalarEvaluator *LandmarkCountHeuristic::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    int lm_type_ = LandmarkCountHeuristic::rpg_sasp;
    bool admissible_ = false;
    bool optimal_ = false;
    bool pref_ = false;

    // "<name>()" or "<name>(<options>)"
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;

        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_bool_option("admissible",
                                          &admissible_,
                                          "get admissible estimate");
            option_parser.add_int_option("lm_type",
                                         &lm_type_,
                                         "landmarks type");
            option_parser.add_bool_option("optimal",
                                          &optimal_,
                                          "optimal cost sharing");
            option_parser.add_bool_option("pref_ops",
                                          &pref_,
                                          "identify preferred operators");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        throw ParseError(start + 1);
    }

    if (dry_run)
        return 0;
    else
        return new LandmarkCountHeuristic(pref_, admissible_, optimal_,
                                          lm_type_);
}
