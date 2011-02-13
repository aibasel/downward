#ifndef LANDMARKS_LANDMARK_GRAPH_H
#define LANDMARKS_LANDMARK_GRAPH_H

#include <vector>
#include <set>
#include <map>
#include <ext/hash_map>
#include <list>
#include <ext/hash_set>
#include <cassert>

#include "../operator.h"
#include "exploration.h"
#include "landmark_types.h"
#include "../option_parser.h"

using namespace __gnu_cxx;

enum edge_type {
    /* NOTE: The code relies on the fact that larger numbers are
       stronger in the sense that, e.g., every greedy-necessary
       ordering is also natural and reasonable. (It is a sad fact of
       terminology that necessary is indeed a special case of
       greedy-necessary, i.e., every necessary ordering is
       greedy-necessary, but not vice versa. */

    necessary = 4,
    greedy_necessary = 3,
    natural = 2,
    reasonable = 1,
    obedient_reasonable = 0
};

enum landmark_status {lm_reached = 0, lm_not_reached = 1, lm_needed_again = 2};

class LandmarkNode {
    int id;
public:
    LandmarkNode(vector<int> &variables, vector<int> &values, bool disj, bool conj = false)
        : id(-1), vars(variables), vals(values), disjunctive(disj), conjunctive(conj), in_goal(false),
          min_cost(1), shared_cost(0.0), status(lm_not_reached),
          effect_of_ununsed_alm(false), is_derived(false) {
    }

    vector<int> vars;
    vector<int> vals;
    bool disjunctive;
    bool conjunctive;
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

    int get_id() const {
        return id;
    }

    void assign_id(int new_id) {
        assert(id == -1 || new_id == id);
        id = new_id;
    }

    bool is_goal() const {
        return in_goal;
    }

    bool is_true_in_state(const State &state) const {
        if (disjunctive) {
            for (unsigned int i = 0; i < vars.size(); i++) {
                if (state[vars[i]] == vals[i]) {
                    return true;
                }
            }
            return false;
        } else {
            for (int i = 0; i < vars.size(); i++) {
                if (state[vars[i]] != vals[i]) {
                    return false;
                }
            }
            return true;
        }
    }

    int get_status(bool exclude_unused_alm_effects) const {
        return exclude_unused_alm_effects && effect_of_ununsed_alm ? lm_reached : status;
    }
};

struct LandmarkNodeComparer {
    bool operator()(LandmarkNode *a, LandmarkNode *b) const {
        if (a->vars.size() > b->vars.size()) {
            return true;
        }
        if (a->vars.size() < b->vars.size()) {
            return false;
        }
        for (int i = 0; i < a->vars.size(); i++) {
            if (a->vars[i] > b->vars[i])
                return true;
            if (a->vars[i] < b->vars[i])
                return false;
        }
        for (int i = 0; i < a->vals.size(); i++) {
            if (a->vals[i] > b->vals[i])
                return true;
            if (a->vals[i] < b->vals[i])
                return false;
        }
        return false;
    }
};


typedef hash_set<LandmarkNode *, hash_pointer> LandmarkSet;

class LandmarkGraph {
public:
    struct Options {
        bool reasonable_orders;
        bool only_causal_landmarks;
        bool disjunctive_landmarks;
        bool conjunctive_landmarks;
        bool no_orders;
        HeuristicOptions heuristic_options;
        int lm_cost_type;

        Options();

        void add_option_to_parser(NamedOptionParser &option_parser);
    };
    struct Pddl_proposition {
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
    
    // ------------------------------------------------------------------------------
    // methods and fields needed only by non-landmarkgraph-factories
    inline int cost_of_landmarks() const {
        return landmarks_cost;
    }
    void count_costs();
    Exploration *get_exploration() const {return exploration; }
    LandmarkNode *get_lm_for_index(int);
    int get_needed_cost() const {
        return needed_cost;
    }
    int get_reached_cost() const {
        return reached_cost;
    }
    bool is_using_reasonable_orderings() const {return reasonable_orders; }
    LandmarkNode *landmark_reached(const pair<int, int> &prop) const;
private:
    int reached_cost;
    int needed_cost;
    Exploration *exploration;

    // ------------------------------------------------------------------------------
    // methods and fields needed by both landmarkgraph-factories and non-landmarkgraph-factories
public:
    inline const set<LandmarkNode *> &get_nodes() const {
        return nodes;
    }
    inline const Operator &get_operator_for_lookup_index(int op_no) const {
        const Operator &op = (op_no < g_operators.size()) ?
        g_operators[op_no] : g_axioms[op_no - g_operators.size()];
        return op;
    }
    inline int number_of_landmarks() const {
        assert(landmarks_count == nodes.size());
        return landmarks_count;
    }
private:
    int landmarks_cost;
    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair> simple_lms_to_nodes;
    hash_map<pair<int, int>, LandmarkNode *, hash_int_pair> disj_lms_to_nodes;

    // ------------------------------------------------------------------------------
    // methods needed only by landmarkgraph-factories
public:
    void rm_landmark_node(LandmarkNode *node);
    void rm_landmark(const pair<int, int> &lmk);

    void print_proposition(const pair<int, int> &fluent) const;
    void read_external_inconsistencies();
    void discard_noncausal_landmarks();
    void discard_disjunctive_landmarks();
    void discard_conjunctive_landmarks();
    void discard_all_orderings();

    void generate();
    bool simple_landmark_exists(const pair<int, int> &lm) const;
    bool disj_landmark_exists(const set<pair<int, int> > &lm) const;
    bool landmark_exists(const pair<int, int> &lm) const;
    bool exact_same_disj_landmark_exists(const set<pair<int, int> > &lm) const;

    void dump_node(const LandmarkNode *node_p) const;
    void dump() const;

    int number_of_disj_landmarks() const {
        return landmarks_count - (simple_lms_to_nodes.size() + conj_lms);
    }
    int number_of_conj_landmarks() const {
        return conj_lms;
    }
    int number_of_edges() const;

    //bool is_dead_end() const {return dead_end_found; }

    //void count_shared_costs();
    
    inline bool inconsistent(const pair<int, int> &a, const pair<int, int> &b) const {
        //if (a.first == b.first && a.second == b.second)
        if (a == b)
            return false;
        if (a.first != b.first || a.second != b.second)
            if (a.first == b.first && a.second != b.second)
                return true;
            if (/*external_inconsistencies_read &&*/
                inconsistent_facts[a.first][a.second].find(b) != inconsistent_facts[a.first][a.second].end())
                return true;
            return false;
    }
    
    // TODO: check how some of the following added methods may be improved. remove comments later
    // added while refactoring
    // methods needed for h_m_landmarks (which no longer inherit landmarks_graph)

    void insert_node(std::pair<int, int> lm, LandmarkNode &node, bool conj);
    
    // 2 methods needed for rpg_sasp
    LandmarkNode &make_disj_node_simple(std::pair<int, int> lm);
    const hash_map<pair<int, int>, Pddl_proposition, hash_int_pair> &get_pddl_propositions() const {
        return pddl_propositions; 
    }
    const map<string, int> &get_pddl_proposition_indices() const {
        return pddl_proposition_indeces;
    }
    
    void edge_add(LandmarkNode &from, LandmarkNode &to, edge_type type);
    
    inline bool relaxed_task_solvable(bool level_out,
                                      const LandmarkNode *exclude,
                                      bool compute_lvl_op = false) const {
                                          vector<vector<int> > lvl_var;
                                          vector<hash_map<pair<int, int>, int, hash_int_pair> > lvl_op;
                                          return relaxed_task_solvable(lvl_var, lvl_op, level_out, exclude, compute_lvl_op);
                                      }
    LandmarkNode &landmark_add_simple(const pair<int, int> &lm);
    
    int relaxed_plan_length_without(LandmarkNode *lm);
    LandmarkNode &landmark_add_disjunctive(const set<pair<int, int> > &lm);
    
    void compute_predecessor_information(LandmarkNode *bp,
                                         vector<vector<int> > &lvl_var,
                                         vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op);
    
    // HACK! (Temporary accessor needed for LandmarkGraphNew.)
    OperatorCost get_lm_cost_type() const {
        return lm_cost_type;
    }
private:   
    bool interferes(const LandmarkNode *, const LandmarkNode *) const;
    bool effect_always_happens(const vector<PrePost> &prepost,
                               set<pair<int, int> > &eff) const;

    void set_landmark_ids();
    void approximate_reasonable_orders(bool obedient_orders);
    void mk_acyclic_graph();
    int loop_acyclic_graph(LandmarkNode &lmn,
                            hash_set<LandmarkNode *, hash_pointer> &acyclic_node_set);
    bool remove_first_weakest_cycle_edge(LandmarkNode *cur,
                                        list<pair<LandmarkNode *, edge_type> > &path,
                                        list<pair<LandmarkNode *, edge_type> >::iterator it);
                                        int calculate_lms_cost() const;

    //bool inconsistent(const pair<int, int> &a, const pair<int, int> &b) const;
    void collect_ancestors(hash_set<LandmarkNode *, hash_pointer> &result, LandmarkNode &node,
                           bool use_reasonable);
    bool relaxed_task_solvable(vector<vector<int> > &lvl_var,
                               vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                               bool level_out,
                               const LandmarkNode *exclude,
                               bool compute_lvl_op = false) const;

    /*bool relaxed_task_solvable_without_operator(vector<vector<int> > &lvl_var,
                                                vector<hash_map<pair<int, int>, int, hash_int_pair> > &lvl_op,
                                                bool level_out,
                                                const Operator *exclude,
                                                bool compute_lvl_op = false) const;*/
    bool is_causal_landmark(const LandmarkNode &landmark) const;

    //void reset_landmarks_count() {landmarks_count = nodes.size(); }
    void calc_achievers();
    
    int landmarks_count;
    int conj_lms;
    
    bool dead_end_found;
    
    vector<vector<set<pair<int, int> > > > inconsistent_facts;
    bool external_inconsistencies_read;
    
    set<LandmarkNode *> nodes;
    vector<LandmarkNode *> ordered_nodes;
    
    hash_map<pair<int, int>, Pddl_proposition, hash_int_pair> pddl_propositions;
    map<string, int> pddl_proposition_indeces; //TODO: make this a hash_map
    
    // methods and fields believed to remain in LandmarkGraph:
public:
    LandmarkGraph(Options &options, Exploration *explor);
    virtual ~LandmarkGraph() {}
    //static void build_lm_graph(LandmarkGraph *lm_graph);
    
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
    inline const vector<int> &get_operators_including_eff(const pair<int, int> &eff) const {
        return operators_eff_lookup[eff.first][eff.second];
    }
    
    //inline const vector<int> &get_operators_including_pre(const pair<int, int> &pre) const {
        //    return operators_pre_lookup[pre.first][pre.second];
        //}
    bool use_orders() const { return !no_orders; }
    
private:
    void generate_operators_lookups();
    
    // fields for construction
    vector<int> empty_pre_operators;
    vector<vector<vector<int> > > operators_eff_lookup;
    vector<vector<vector<int> > > operators_pre_lookup;
    
    bool reasonable_orders;
    bool only_causal_landmarks;
    bool disjunctive_landmarks;
    bool conjunctive_landmarks;
    bool no_orders;
    
    OperatorCost lm_cost_type;
};

#endif
