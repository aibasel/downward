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
    LandmarkNode(std::vector<int> &variables, std::vector<int> &values, bool disj, bool conj = false)
        : id(-1), vars(variables), vals(values), disjunctive(disj), conjunctive(conj), in_goal(false),
          min_cost(1), shared_cost(0.0), status(lm_not_reached),
          is_derived(false) {
    }

    std::vector<int> vars;
    std::vector<int> vals;
    bool disjunctive;
    bool conjunctive;
    __gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer> parents;
    __gnu_cxx::hash_map<LandmarkNode *, edge_type, hash_pointer> children;
    bool in_goal;
    int min_cost; // minimal cost of achieving operators
    double shared_cost;

    landmark_status status;
    bool is_derived;

    __gnu_cxx::hash_set<std::pair<int, int>, hash_int_pair> forward_orders;
    std::set<int> first_achievers;
    std::set<int> possible_achievers;

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
        } else { // conjunctive or simple
            for (int i = 0; i < vars.size(); i++) {
                if (state[vars[i]] != vals[i]) {
                    return false;
                }
            }
            return true;
        }
    }

    int get_status() const {
        return status;
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


typedef __gnu_cxx::hash_set<LandmarkNode *, hash_pointer> LandmarkSet;

class LandmarkGraph {
public:
    static void add_options_to_parser(OptionParser &parser);

    // ------------------------------------------------------------------------------
    // methods needed only by non-landmarkgraph-factories
    inline int cost_of_landmarks() const {return landmarks_cost; }
    void count_costs();
    LandmarkNode *get_lm_for_index(int);
    int get_needed_cost() const {return needed_cost; }
    int get_reached_cost() const {return reached_cost; }
    LandmarkNode *get_landmark(const std::pair<int, int> &prop) const;

    // TODO: the following method should not exist. Ideally, we want the
    // information about support for conditional effects to reside in the
    // landmark factory classes. For now, this cannot easily be done since the
    // factories do not exist anymore when the landmark heuristic is
    // constructed.
    bool supports_conditional_effects() {return conditional_effects_supported; }

    // ------------------------------------------------------------------------------
    // methods needed by both landmarkgraph-factories and non-landmarkgraph-factories
    inline const std::set<LandmarkNode *> &get_nodes() const {
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
    Exploration *get_exploration() const {return exploration; }
    bool is_using_reasonable_orderings() const {return reasonable_orders; }

    // ------------------------------------------------------------------------------
    // methods needed only by landmarkgraph-factories
    LandmarkGraph(const Options &opts);
    virtual ~LandmarkGraph() {}

    inline LandmarkNode &get_simple_lm_node(const std::pair<int, int> &a) const {
        assert(simple_landmark_exists(a));
        return *(simple_lms_to_nodes.find(a)->second);
    }
    inline LandmarkNode &get_disj_lm_node(const std::pair<int, int> &a) const {
        // Note: this only works because every proposition appears in only one disj. LM
        assert(!simple_landmark_exists(a));
        assert(disj_lms_to_nodes.find(a) != disj_lms_to_nodes.end());
        return *(disj_lms_to_nodes.find(a)->second);
    }
    inline const std::vector<int> &get_operators_including_eff(const std::pair<int, int> &eff) const {
        return operators_eff_lookup[eff.first][eff.second];
    }

    bool use_orders() const {return !no_orders; }  // only needed by HMLandmark
    bool use_only_causal_landmarks() const {return only_causal_landmarks; }
    bool use_disjunctive_landmarks() const {return disjunctive_landmarks; }
    bool use_conjunctive_landmarks() const {return conjunctive_landmarks; }

    int number_of_disj_landmarks() const {
        return landmarks_count - (simple_lms_to_nodes.size() + conj_lms);
    }
    int number_of_conj_landmarks() const {
        return conj_lms;
    }
    int number_of_edges() const;

    // HACK! (Temporary accessor needed for LandmarkFactorySasp.)
    OperatorCost get_lm_cost_type() const {
        return lm_cost_type;
    }

    bool simple_landmark_exists(const std::pair<int, int> &lm) const; // not needed by HMLandmark
    bool disj_landmark_exists(const std::set<std::pair<int, int> > &lm) const; // not needed by HMLandmark
    bool landmark_exists(const std::pair<int, int> &lm) const; // not needed by HMLandmark
    bool exact_same_disj_landmark_exists(const std::set<std::pair<int, int> > &lm) const;

    LandmarkNode &landmark_add_simple(const std::pair<int, int> &lm);
    LandmarkNode &landmark_add_disjunctive(const std::set<std::pair<int, int> > &lm);
    LandmarkNode &landmark_add_conjunctive(const std::set<std::pair<int, int> > &lm);
    void rm_landmark_node(LandmarkNode *node);
    LandmarkNode &make_disj_node_simple(std::pair<int, int> lm); // only needed by LandmarkFactorySasp
    void set_landmark_ids();
    void set_landmark_cost(int cost) {
        landmarks_cost = cost;
    }
    void dump_node(const LandmarkNode *node_p) const;
    void dump() const;
private:
    void generate_operators_lookups();
    Exploration *exploration;
    int landmarks_count;
    int conj_lms;
    bool reasonable_orders;
    bool only_causal_landmarks;
    bool disjunctive_landmarks;
    bool conjunctive_landmarks;
    bool no_orders;
    OperatorCost lm_cost_type;
    bool conditional_effects_supported;
    int reached_cost;
    int needed_cost;
    int landmarks_cost;
    __gnu_cxx::hash_map<std::pair<int, int>, LandmarkNode *, hash_int_pair> simple_lms_to_nodes;
    __gnu_cxx::hash_map<std::pair<int, int>, LandmarkNode *, hash_int_pair> disj_lms_to_nodes;
    std::set<LandmarkNode *> nodes;
    std::vector<LandmarkNode *> ordered_nodes;
    std::vector<std::vector<std::vector<int> > > operators_eff_lookup;
};

#endif
