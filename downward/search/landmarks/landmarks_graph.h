#ifndef LANDMARKS_LANDMARKS_GRAPH_H
#define LANDMARKS_LANDMARKS_GRAPH_H

#include <vector>
#include <set>
#include <map>
#include <ext/hash_map>
#include <list>
#include <ext/hash_set>
#include <cassert>

#include "../operator.h"
#include "exploration.h"
#include "landmarks_types.h"

using namespace __gnu_cxx;

enum edge_type {n = 4, gn = 3, r = 1, o_r = 0, ln = 2};
enum landmark_status {lm_reached = 0, lm_not_reached = 1, lm_needed_again = 2};

static int last_lm_id = 0;

class LandmarkNode {
public:
    LandmarkNode(vector<int> &variables, vector<int> &values, bool disj)
        : vars(variables), vals(values), disjunctive(disj), in_goal(false),
          min_cost(1), shared_cost(0.0), status(lm_not_reached),
          effect_of_ununsed_alm(false), is_derived(false) {
        id = last_lm_id;
        last_lm_id++;
    }

    int id;

    vector<int> vars;
    vector<int> vals;
    bool disjunctive;
    hash_map<LandmarkNode *, edge_type, hash_pointer> parents;
    hash_map<LandmarkNode *, edge_type, hash_pointer> children;
    bool in_goal;
    int min_cost; // minimal cost of achieving operators
    double shared_cost;

    landmark_status status;
    bool effect_of_ununsed_alm;
    bool is_derived;

    hash_set<pair<int, int>, hash_int_pair> forward_orders;
    set<int> first_achievers;
    set<int> possible_achievers;

    bool is_goal() const {return in_goal; }
    bool is_true_in_state(const State &state) const {
        if (!disjunctive) {
            return state[vars[0]] == vals[0];
        } else {
            bool is_true = false;
            for (unsigned int i = 0; i < vars.size(); i++) {
                if (state[vars[i]] == vals[i]) {
                    is_true = true;
                    break;
                }
            }
            return is_true;
        }
    }

    int get_status(bool exclude_unused_alm_effects) {
        return exclude_unused_alm_effects && effect_of_ununsed_alm ? 0 : status;
    }
};

typedef hash_set<LandmarkNode *, hash_pointer> LandmarkSet;
typedef set<const Operator *> ActionLandmarkSet;

class LandmarksGraph {
public:
    class Pddl_proposition {
public:
        string predicate;
        vector<string> arguments;
        string to_string() const {
            string output = predicate;
            for (unsigned int i = 0; i < arguments.size(); i++) {
                output += " ";
                output += arguments[i];
            }
            return output;
        }
    };
    void rm_landmark_node(LandmarkNode *node);
    void rm_landmark(const pair<int, int> &lmk);
    LandmarksGraph(Exploration *explor);
    virtual ~LandmarksGraph() {}
    void read_external_inconsistencies();
    void use_reasonable_orders() {reasonable_orders = true; }
    void discard_disjunctive_landmarks();

    void generate();
    bool simple_landmark_exists(const pair<int, int> &lm) const;
    bool disj_landmark_exists(const set<pair<int, int> > &lm) const;
    bool landmark_exists(const pair<int, int> &lm) const;
    bool exact_same_disj_landmark_exists(const set<pair<int, int> > &lm) const;

    LandmarkNode *landmark_reached(const pair<int, int> &prop) const;

    void dump_node(const LandmarkNode *node_p) const;
    void dump() const;
    inline int cost_of_landmarks() const {
        return landmarks_cost;
    }
    inline int number_of_landmarks() const {
        assert(landmarks_count == nodes.size());
        return landmarks_count;
    }
    int number_of_disj_landmarks() const {
        return landmarks_count - simple_lms_to_nodes.size();
    }
    int number_of_edges() const;

    inline LandmarkNode &get_simple_lm_node(const pair<int, int> &a) const {
        assert(simple_landmark_exists(a));
        return *(simple_lms_to_nodes.find(a)->second);
    }

    inline LandmarkNode &get_disj_lm_node(const pair<int, int> &a) const {
        // Note: this only works because every proposition appears in only one disj. LM
        assert(!simple_landmark_exists(a));
        assert(disj_lms_to_nodes.find(a) != disj_lms_to_nodes.end());
        return *(disj_lms_to_nodes.find(a)->second);
    }

    inline const set<LandmarkNode *> &get_nodes() const {
        return nodes;
    }

    inline const vector<int> &get_operators_including_eff(const pair<int, int> &eff) const {
        return operators_eff_lookup[eff.first][eff.second];
    }

    inline const vector<int> &get_operators_including_pre(const pair<int, int> &pre) const {
        return operators_pre_lookup[pre.first][pre.second];
    }
    inline const Operator &get_operator_for_lookup_index(int op_no) const {
        const Operator &op = (op_no < g_operators.size()) ?
                             g_operators[op_no] : g_axioms[op_no - g_operators.size()];
        return op;
    }

    inline const set<const Operator *> &get_action_landmarks() const {
        return action_landmarks;
    }

    double shared_cost_of_landmarks() const {return shared_landmarks_cost; }
    int get_reached_cost() const {return reached_cost; }
    int get_needed_cost() const {return needed_cost; }
    double get_reached_shared_cost() const {return reached_shared_cost; }
    double get_needed_shared_cost() const {return needed_shared_cost; }
    double get_not_unused_alm_effect_reached_shared_cost() const {
        return not_unused_alm_effect_reached_shared_cost;
    }
    double get_not_unused_alm_effect_needed_shared_cost() const {
        return not_unused_alm_effect_needed_shared_cost;
    }
    int get_unused_action_landmark_cost() const {return unused_action_landmark_cost; }
    void set_unused_action_landmark_cost(int cost) {unused_action_landmark_cost = cost; }
    bool is_dead_end() const {return dead_end_found; }

    void count_shared_costs();
    void count_costs();
private:
    Exploration *exploration;
    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const vector<PrePost> &prepost,
                               set<pair<int, int> > &eff) const;
    virtual void generate_landmarks() = 0;
    vector<int> empty_pre_operators;
    vector<vector<vector<int> > > operators_eff_lookup;
    vector<vector<vector<int> > > operators_pre_lookup;
    void generate_operators_lookups();
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                           hash_set<LandmarkNode *, hash_pointer> &acyclic_node_set);

    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                         list<pair<LandmarkNode *, edge_type> > &path,
                                         list<pair<LandmarkNode *, edge_type> >::iterator it);

protected:
    int landmarks_count;

    double shared_landmarks_cost;
    int landmarks_cost;

    int reached_cost;
    double reached_shared_cost;
    double not_unused_alm_effect_reached_shared_cost;

    int needed_cost;
    double needed_shared_cost;
    double not_unused_alm_effect_needed_shared_cost;

    int unused_action_landmark_cost;
    bool dead_end_found;
private:
    int calculate_lms_cost() const;
    bool use_external_inconsistencies;
    bool reasonable_orders;

    vector<vector<set<pair<int, int> > > > inconsistent_facts;

protected:

    set<LandmarkNode *> nodes;
    set<const Operator *> action_landmarks;

    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair> simple_lms_to_nodes;
    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair> disj_lms_to_nodes;

    hash_map<pair<int, int>, Pddl_proposition, hash_int_pair> pddl_propositions;
    map<string, int> pddl_proposition_indeces; //TODO: make this a hash_map

    bool inconsistent(const pair<int, int> &a, const pair<int, int> &b) const;
    void collect_ancestors(hash_set<LandmarkNode *, hash_pointer> &result, LandmarkNode &node,
                           bool use_reasonable);
    inline bool relaxed_task_solvable(bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
        vector<vector<int> > lvl_var;
        vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
        return relaxed_task_solvable(lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
    }
    bool relaxed_task_solvable(vector<vector<int> > &lvl_var,
                               vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;

    bool relaxed_task_solvable_without_operator(vector<vector<int> > &lvl_var,
                                                vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                                                bool level_out,
                                                const Operator *exclude,
                                                bool compute_lvl_op = false) const;

    void compute_predecessor_information(LandmarkNode *bp,
                                         vector<vector<int> > &lvl_var,
                                         vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op);

    int relaxed_plan_length_without(LandmarkNode *lm);

    LandmarkNode &landmark_add_simple(const pair<int, int> &lm);
    LandmarkNode &landmark_add_disjunctive(const set<pair<int, int> > &lm);
    void edge_add(LandmarkNode &from, LandmarkNode &to,
                  edge_type type);

    void generate_action_landmarks();
    bool check_action_landmark(const Operator *op);

    void reset_landmarks_count() {landmarks_count = nodes.size(); }
    virtual void calc_achievers();
};

#endif
