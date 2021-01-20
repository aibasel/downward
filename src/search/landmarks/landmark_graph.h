#ifndef LANDMARKS_LANDMARK_GRAPH_H
#define LANDMARKS_LANDMARK_GRAPH_H

#include "../global_state.h"
#include "../task_proxy.h"

#include "../utils/hash.h"

#include <cassert>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace landmarks {
enum class EdgeType {
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

class LandmarkNode {
    int id;
public:
    LandmarkNode(std::vector<FactPair> &facts, bool disj, bool conj = false)
        : id(-1), facts(facts), disjunctive(disj), conjunctive(conj), in_goal(false),
          min_cost(1), shared_cost(0.0), is_derived(false) {
    }

    std::vector<FactPair> facts;
    bool disjunctive;
    bool conjunctive;
    std::unordered_map<LandmarkNode *, EdgeType> parents;
    std::unordered_map<LandmarkNode *, EdgeType> children;
    bool in_goal;
    int min_cost; // minimal cost of achieving operators
// Does not belong here
    double shared_cost;

// What does this mean?
    bool is_derived;

// What does this mean?
    utils::HashSet<FactPair> forward_orders;
    std::set<int> first_achievers;
    std::set<int> possible_achievers;

    // No implementations in header

    int get_id() const {
        return id;
    }

    // Possibly should not be changable
    void assign_id(int new_id) {
        assert(id == -1 || new_id == id);
        id = new_id;
    }

    bool is_goal() const {
        return in_goal;
    }

    bool is_true_in_state(const GlobalState &global_state) const {
        if (disjunctive) {
            for (const FactPair &fact : facts) {
                if (global_state[fact.var] == fact.value) {
                    return true;
                }
            }
            return false;
        } else { // conjunctive or simple
            for (const FactPair &fact : facts) {
                if (global_state[fact.var] != fact.value) {
                    return false;
                }
            }
            return true;
        }
    }

    bool is_true_in_state(const State &state) const {
        if (disjunctive) {
            for (const FactPair &fact : facts) {
                if (state[fact.var].get_value() == fact.value) {
                    return true;
                }
            }
            return false;
        } else { // conjunctive or simple
            for (const FactPair &fact : facts) {
                if (state[fact.var].get_value() != fact.value) {
                    return false;
                }
            }
            return true;
        }
    }
};

// Only used once
struct LandmarkNodeComparer {
    bool operator()(LandmarkNode *a, LandmarkNode *b) const {
        if (a->facts.size() != b->facts.size())
            return a->facts.size() > b->facts.size();
        for (size_t i = 0; i < a->facts.size(); ++i) {
            if (a->facts[i].var != b->facts[i].var)
                return a->facts[i].var > b->facts[i].var;
        }
        for (size_t i = 0; i < a->facts.size(); ++i) {
            if (a->facts[i].value != b->facts[i].value)
                return a->facts[i].value > b->facts[i].value;
        }
        return false;
    }
};


using LandmarkSet = std::unordered_set<const LandmarkNode *>;

class LandmarkGraph {
public:
    using Nodes = std::vector<std::unique_ptr<LandmarkNode>>;
    // ------------------------------------------------------------------------------
    // methods needed only by non-landmarkgraph-factories

    LandmarkNode *get_lm_for_index(int) const;
    LandmarkNode *get_landmark(const FactPair &fact) const;

    // ------------------------------------------------------------------------------
    // methods needed by both landmarkgraph-factories and non-landmarkgraph-factories

    // Redundant?
    inline const Nodes &get_nodes() const {
        return nodes;
    }

    inline int number_of_landmarks() const {
        return nodes.size();
    }

    // ------------------------------------------------------------------------------
    // methods needed only by landmarkgraph-factories
    explicit LandmarkGraph(const TaskProxy &task_proxy);

    inline LandmarkNode &get_simple_lm_node(const FactPair &a) const {
        assert(simple_landmark_exists(a));
        return *(simple_lms_to_nodes.find(a)->second);
    }
    inline LandmarkNode &get_disj_lm_node(const FactPair &a) const {
        // Note: this only works because every proposition appears in only one disj. LM
        assert(!simple_landmark_exists(a));
        assert(disj_lms_to_nodes.find(a) != disj_lms_to_nodes.end());
        return *(disj_lms_to_nodes.find(a)->second);
    }
    // Does not belong here
    inline const std::vector<int> &get_operators_including_eff(const FactPair &eff) const {
        return operators_eff_lookup[eff.var][eff.value];
    }

    int number_of_disj_landmarks() const {
        return disj_lms;
    }
    int number_of_conj_landmarks() const {
        return conj_lms;
    }
    int number_of_edges() const;

    bool simple_landmark_exists(const FactPair &lm) const; // not needed by HMLandmark
    bool disj_landmark_exists(const std::set<FactPair> &lm) const;  // not needed by HMLandmark
    bool landmark_exists(const FactPair &lm) const; // not needed by HMLandmark
    bool exact_same_disj_landmark_exists(const std::set<FactPair> &lm) const;

    LandmarkNode &landmark_add_simple(const FactPair &lm);
    LandmarkNode &landmark_add_disjunctive(const std::set<FactPair> &lm);
    LandmarkNode &landmark_add_conjunctive(const std::set<FactPair> &lm);
    // rather if (...) remove node
    void remove_node_if(const std::function<bool (const LandmarkNode &)> &remove_node);
    LandmarkNode &make_disj_node_simple(const FactPair &lm); // only needed by LandmarkFactorySasp
    void set_landmark_ids();
    // Possible without VariablesProxy?
    void dump_node(const VariablesProxy &variables, const LandmarkNode *node_p) const;
    void dump(const VariablesProxy &variables) const;
private:
    // Do we need this (here)?
    void generate_operators_lookups(const TaskProxy &task_proxy);
    void remove_node_occurrences(LandmarkNode *node);
    // Not clear, same for disjunctive and simple?
    int conj_lms, disj_lms;

    // Remove cost members
    utils::HashMap<FactPair, LandmarkNode *> simple_lms_to_nodes;
    utils::HashMap<FactPair, LandmarkNode *> disj_lms_to_nodes;
    Nodes nodes;
    std::vector<std::vector<std::vector<int>>> operators_eff_lookup;
};
}

#endif
