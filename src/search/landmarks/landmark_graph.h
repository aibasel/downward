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
    /*
      NOTE: The code relies on the fact that larger numbers are stronger in the
      sense that, e.g., every greedy-necessary ordering is also natural and
      reasonable. (It is a sad fact of terminology that necessary is indeed a
      special case of greedy-necessary, i.e., every necessary ordering is
      greedy-necessary, but not vice versa.
    */
    NECESSARY = 4,
    GREEDY_NECESSARY = 3,
    NATURAL = 2,
    REASONABLE = 1,
    OBEDIENT_REASONABLE = 0
};

class LandmarkNode {
    int id;
public:
    LandmarkNode(std::vector<FactPair> &facts, bool disjunctive, bool conjunctive)
        : id(-1), facts(facts), disjunctive(disjunctive), conjunctive(conjunctive),
          is_true_in_goal(false), cost(1), is_derived(false) {
    }

    std::vector<FactPair> facts;
    bool disjunctive;
    bool conjunctive;
    std::unordered_map<LandmarkNode *, EdgeType> parents;
    std::unordered_map<LandmarkNode *, EdgeType> children;
    bool is_true_in_goal;

    // Cost of achieving the landmark (as determined by the landmark factory)
    int cost;

    bool is_derived;

    std::set<int> first_achievers;
    std::set<int> possible_achievers;

    int get_id() const {
        return id;
    }

    // TODO: Should possibly not be changeable
    void set_id(int new_id) {
        assert(id == -1 || new_id == id);
        id = new_id;
    }

    bool is_true_in_state(const GlobalState &global_state) const;
    bool is_true_in_state(const State &state) const;
};

using LandmarkSet = std::unordered_set<const LandmarkNode *>;

class LandmarkGraph {
public:
    /*
      TODO: get rid of this by removing get_nodes() and instead offering
      functions begin() and end() with an iterator class, so users of the
      LandmarkGraph can do loops like this:
        for (const LandmarkNode &n : graph) {...}
     */
    using Nodes = std::vector<std::unique_ptr<LandmarkNode>>;
private:
    int num_conjunctive_landmarks;
    int num_disjunctive_landmarks;

    utils::HashMap<FactPair, LandmarkNode *> simple_landmarks_to_nodes;
    utils::HashMap<FactPair, LandmarkNode *> disjunctive_landmarks_to_nodes;
    Nodes nodes;
    const TaskProxy &task_proxy;

    void remove_node_occurrences(LandmarkNode *node);
public:
    /* The following methods are needed only by non-landmarkgraph-factories. */
    LandmarkNode *get_lm_for_index(int) const;
    LandmarkNode *get_landmark(const FactPair &fact) const;

    /* The following methods are needed by both landmarkgraph-factories and
       non-landmarkgraph-factories. */
    inline const Nodes &get_nodes() const {
        return nodes;
    }
    inline int number_of_landmarks() const {
        return nodes.size();
    }

    /* The following methods are needed only by landmarkgraph-factories. */
    explicit LandmarkGraph(const TaskProxy &task_proxy);
    inline LandmarkNode &get_simple_lm_node(const FactPair &a) const {
        assert(contains_simple_landmark(a));
        return *(simple_landmarks_to_nodes.find(a)->second);
    }
    inline LandmarkNode &get_disj_lm_node(const FactPair &a) const {
        /* Note: this only works because every proposition appears in only one
           disjunctive landmark. */
        assert(!contains_simple_landmark(a));
        assert(contains_disjunctive_landmark(a));
        return *(disjunctive_landmarks_to_nodes.find(a)->second);
    }

    int number_of_disj_landmarks() const {
        return num_disjunctive_landmarks;
    }
    int number_of_conj_landmarks() const {
        return num_conjunctive_landmarks;
    }
    int number_of_edges() const;

    // not needed by HMLandmark
    bool contains_simple_landmark(const FactPair &lm) const;
    // not needed by HMLandmark
    bool contains_disjunctive_landmark(const FactPair &lm) const;
    bool contains_overlapping_disjunctive_landmark(const std::set<FactPair> &lm) const;
    bool contains_identical_disjunctive_landmark(const std::set<FactPair> &lm) const;
    // not needed by HMLandmark
    bool contains_landmark(const FactPair &fact) const;

    LandmarkNode &add_simple_landmark(const FactPair &lm);
    LandmarkNode &add_disjunctive_landmark(const std::set<FactPair> &lm);
    LandmarkNode &add_conjunctive_landmark(const std::set<FactPair> &lm);
    void remove_node_if(
        const std::function<bool (const LandmarkNode &)> &remove_node);

    // only needed by LandmarkFactorySasp
    LandmarkNode &make_disj_node_simple(const FactPair &lm);
    void set_landmark_ids();
};
}

#endif
