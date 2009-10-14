#include <climits>
#include <math.h>

#include "landmarks_count_heuristic.h"
#include "../globals.h"
#include "../operator.h"
#include "../search_engine.h"
#include "../successor_generator.h"
#include "landmarks_graph_rpg_sasp.h"
#include "landmarks_graph_zhu_givan.h"
#include "landmarks_graph_rpg_exhaust.h"
#include "../timer.h"

LandmarksGraph *g_lgraph; // global to be accessible by state

static void init_lm_graph(Exploration* exploration, int landmarks_type) {
    switch (landmarks_type) {
    case 0:
        g_lgraph = new LandmarksGraphNew(exploration);
        break;
    case 1:
        g_lgraph = new LandmarksGraphZhuGivan(exploration); // Note: remove exploration here?
        break;
    case 2:
        g_lgraph = new LandmarksGraphExhaust(exploration);
        break;
    default:
        assert(false); // unknown landmarks_type
    }
}

static LandmarksGraph *build_landmarks_graph(Exploration* exploration) {
    bool reasonable_orders = false; // option to use/not use reasonable orderings
    bool disjunctive_lms = false; // option to discard/not discard disj. landmarks before search
    if (g_lgraph != NULL) // in case of iterated search LM graph has already been constructed
        return g_lgraph;
    Timer lm_generation_timer;
    enum {
        rpg_sasp, zhu_givan, exhaust
    } landmarks_type = rpg_sasp; // change for different landmarks
    init_lm_graph(exploration, landmarks_type);
    g_lgraph->read_external_inconsistencies();
    if (reasonable_orders) {
        g_lgraph->use_reasonable_orders();
    }
    g_lgraph->generate();
    cout << "Landmarks generation time: " << lm_generation_timer << endl;
    if (g_lgraph->number_of_landmarks() == 0)
        cout << "Warning! No landmarks found. Task unsolvable?" << endl;
    cout << "Generated " << g_lgraph->number_of_landmarks()
            << " landmarks, of which " << g_lgraph->number_of_disj_landmarks()
            << " are disjunctive" << endl << "          "
            << g_lgraph->number_of_edges() << " edges\n";
    if (!disjunctive_lms)
        g_lgraph->discard_disjunctive_landmarks();
    //g_lgraph->dump();
    return g_lgraph;
}

LandmarksCountHeuristic::LandmarksCountHeuristic(bool preferred_ops,
        bool admissible) :
    exploration(new Exploration), lgraph(*build_landmarks_graph(exploration)),
            lm_status_manager(lgraph) {

    cout << "Initializing landmarks count heuristic..." << endl;
    preferred_operators = preferred_ops;
    lookahead = INT_MAX;
    // When generating preferred operators, we plan towards
    // non-disjunctive landmarks only
    ff_search_disjunctive_lms = false;
    // Turn goal into hash set
    goal.resize(g_goal.size());
    for (int i = 0; i < g_goal.size(); i++)
        goal.insert(make_pair(g_goal[i].first, g_goal[i].second));

    lm_cost_assignment = new LandmarkUniformSharedCostAssignment(lgraph, true);

    if (admissible) {
        use_shared_cost = true;
        use_dynamic_cost_sharing = true;
        use_action_landmark_count = true;
    } else {
        use_shared_cost = false;
        use_dynamic_cost_sharing = false;
        use_action_landmark_count = false;
    }
}

void LandmarksCountHeuristic::set_recompute_heuristic(const State& state) {
    if (preferred_operators) {
        assert(exploration != 0);
        // Set additional goals for FF exploration
        vector<pair<int, int> > lm_leaves;
        //hash_set<const LandmarkNode*, hash_pointer> result;
        //check_partial_plan(result);

        LandmarkSet& result = lm_status_manager.get_reached_landmarks(state);

        collect_lm_leaves(ff_search_disjunctive_lms, result, lm_leaves);
        exploration->set_additional_goals(lm_leaves);
    }
}

int LandmarksCountHeuristic::get_heuristic_value(const State& state) {
    // Get landmarks that have been true at some point (put into
    // "reached_lms") and their cost

    int h = 0;
    //const State& state = node.get_state();

    bool goal_reached = true;
    for (int i = 0; i < g_goal.size(); i++)
        if (state[g_goal[i].first] != g_goal[i].second)
            goal_reached = false;

    if (goal_reached) {
        return 0;
    }

    bool dead_end = lm_status_manager.update_lm_status(state);

    if (dead_end) {
        return DEAD_END;
        //return 1;
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
            reached_cost
                    = lgraph.get_not_unused_alm_effect_reached_shared_cost();
            needed_cost = lgraph.get_not_unused_alm_effect_needed_shared_cost();
        } else {
            reached_cost = lgraph.get_reached_shared_cost();
            needed_cost = lgraph.get_needed_shared_cost();
        }
    } else {
        total_cost = (double) lgraph.cost_of_landmarks();
        reached_cost = (double) lgraph.get_reached_cost();
        needed_cost = (double) lgraph.get_needed_cost();
    }

    //assert(reached_cost == needed_cost);

    h = ceil(total_cost - reached_cost + needed_cost + additional_cost
            - epsilon);
    /*
     if (g_verbose) {
     cout << "h = " << total_cost << " - " << reached_cost << " + " << needed_cost
     << " + " << additional_cost << endl;
     }
     */

    assert(-1 * epsilon <= needed_cost);
    //assert(reached_cost + additional_cost - needed_cost >= -1 * epsilon);
    assert(reached_cost - needed_cost >= -1 * epsilon);
    assert(h >= 0);

    // Test if goal has been reached even though the landmark heuristic is
    // not 0. This may happen if landmarks are achieved before their parents
    // in the landmarks graph, because they do not get counted as reached
    // in that case. However, we must return 0 for a goal state.

    if (goal_reached && h != 0) {
        cout << "Goal reached but Landmark heuristic != 0" << endl;
        /*
         cout << "Never accepted the following Lms:" << endl;
         const set<LandmarkNode*>& my_nodes = lgraph.get_nodes();
         set<LandmarkNode*>::const_iterator it;
         for(it = my_nodes.begin(); it != my_nodes.end(); ++it) {
         const LandmarkNode& node = **it;
         if(reached_lms.find(&node) == reached_lms.end())
         lgraph.dump_node(&node);
         }
         */
        cout << "LM heuristic was " << h << " - returning zero" << endl;
        return 0;
    }

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
        h = max;
    }

    return h;
}

int LandmarksCountHeuristic::compute_heuristic(const State &state) {
    int h = get_heuristic_value(state);

    // For now: assume that we always do the relaxed exploration rather than
    // re-using any possible previous effort by some other heuristic.
    // TODO: get rid of double "check_partial_plan"
    set_recompute_heuristic(state);
    // Get landmarks that have been true at some point (put into
    // "reached_lms") and their cost
    LandmarkSet &reached_lms = lm_status_manager.get_reached_landmarks(state);
    const int reached_lms_cost = lgraph.get_reached_cost();
    //const int reached_lms_cost = check_partial_plan(reached_lms);
    // Get landmarks that are needed again (of those in
    // "reached_lms") because they have been made false in the meantime,
    // but are goals or required by unachieved successors
    //LandmarkSet needed_lms;
    //const int needed_lms_cost = get_needed_landmarks(state, needed_lms);
    //assert(0 <= needed_lms_cost);
    //assert(reached_lms_cost >= needed_lms_cost);
    //assert(reached_lms.size() >= needed_lms.size());
    // Heuristic is total number of landmarks,
    // minus the ones we have already achieved and do not need again

    //h = lgraph.number_of_landmarks() - reached_lms.size() + needed_lms.size();
    assert(h >= 0);

    // Test if goal has been reached even though the landmark heuristic is
    // not 0. This may happen if landmarks are achieved before their parents
    // in the landmarks graph, because they do not get counted as reached
    // in that case. However, we must return 0 for a goal state.
    bool goal_reached = true;
    for (int i = 0; i < g_goal.size(); i++)
        if (state[g_goal[i].first] != g_goal[i].second)
            goal_reached = false;
    if (goal_reached && h != 0) {
        cout << "Goal reached but Landmark heuristic != 0" << endl;
        /*
         cout << "Never accepted the following Lms:" << endl;
         const set<LandmarkNode*>& my_nodes = lgraph.get_nodes();
         set<LandmarkNode*>::const_iterator it;
         for(it = my_nodes.begin(); it != my_nodes.end(); ++it) {
         const LandmarkNode& node = **it;
         if(reached_lms.find(&node) == reached_lms.end())
         lgraph.dump_node(&node);
         }
         */
        cout << "LM heuristic was " << h << " - returning zero" << endl;
        return 0;
    }

#ifndef NDEBUG
    // For debugging purposes, check whether heuristic is 0 even though
    // goal is not reached. This should never happen.
    if (h == 0 && !goal_reached) {
        cout << "WARNING! Landmark heuristic is 0, but goal not reached"
                << endl;
        for (int i = 0; i < g_goal.size(); i++)
            if (state[g_goal[i].first] != g_goal[i].second) {
                cout << "missing goal prop "
                        << g_variable_name[g_goal[i].first] << " : "
                        << g_goal[i].second << endl;
            }
        //cout << reached_lms_cost << " " << needed_lms_cost << endl;
    }
#endif

    if (!preferred_operators || h == 0) {// no (need for) helpful actions, return
        return h;
    }

    // Try generating helpful actions (those that lead to new leaf LM in the
    // next step). If all Lms have been reached before or no new ones can be
    // reached within next step, helpful actions are those occuring in a PLAN
    // to achieve one of the LM leaves.

    if (reached_lms_cost == lgraph.cost_of_landmarks()
            || !generate_helpful_actions(state, reached_lms)) {

        assert(exploration != NULL);
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

void LandmarksCountHeuristic::collect_lm_leaves(bool disjunctive_lms,
        LandmarkSet& reached_lms, vector<pair<int, int> >& leaves) {

    set<LandmarkNode*>::const_iterator it;
    for (it = lgraph.get_nodes().begin(); it != lgraph.get_nodes().end(); it++) {
        LandmarkNode* node_p = *it;

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

int LandmarksCountHeuristic::ff_search_lm_leaves(bool disjunctive_lms,
        const State& state, LandmarkSet& reached_lms) {

    vector<pair<int, int> > leaves;
    collect_lm_leaves(disjunctive_lms, reached_lms, leaves);
    if (exploration->plan_for_disj(leaves, state) == DEAD_END) {
        return DEAD_END;
    } else
        return 0;
}

bool LandmarksCountHeuristic::check_node_orders_disobeyed(LandmarkNode& node,
        const LandmarkSet& reached) const {

    const hash_map<LandmarkNode*, edge_type, hash_pointer>& parents =
            node.parents;
    for (hash_map<LandmarkNode*, edge_type, hash_pointer>::const_iterator
            parent_it = parents.begin(); parent_it != parents.end(); parent_it++) {
        LandmarkNode& parent = *(parent_it->first);
        if (reached.find(&parent) == reached.end()) {
            return true;
        }
    }
    return false;
}

bool LandmarksCountHeuristic::generate_helpful_actions(const State& state,
        const LandmarkSet& reached) {

    /* Find actions that achieve new landmark leaves. If no such action exist,
     return false. If a simple landmark can be achieved, return only operators
     that achieve simple landmarks, else return operators that achieve
     disjunctive landmarks */
    vector<const Operator *> all_operators;
    g_successor_generator->generate_applicable_ops(state, all_operators);
    vector<const Operator *> ha_simple;
    vector<const Operator *> ha_disj;

    for (int i = 0; i < all_operators.size(); i++) {
        const vector<PrePost>& prepost = all_operators[i]->get_pre_post();
        for (int j = 0; j < prepost.size(); j++) {
            if (!prepost[j].does_fire(state))
                continue;
            const pair<int, int> varval = make_pair(prepost[j].var,
                    prepost[j].post);
            LandmarkNode* lm_p = lgraph.landmark_reached(varval);
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

bool LandmarksCountHeuristic::landmark_is_interesting(const State& s,
        const LandmarkSet& reached, LandmarkNode& lm) const {
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

void LandmarksCountHeuristic::initialize() {
    lm_status_manager.set_landmarks_for_initial_state(*g_initial_state);
}

bool LandmarksCountHeuristic::reach_state(const State& parent_state,
        const Operator &op, const State& state) {
    lm_status_manager.update_reached_lms(parent_state, op, state);

    return true;
}
