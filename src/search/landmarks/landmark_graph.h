#ifndef LANDMARKS_LANDMARK_GRAPH_H
#define LANDMARKS_LANDMARK_GRAPH_H

#include "landmark.h"

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
    NECESSARY = 3,
    GREEDY_NECESSARY = 2,
    NATURAL = 1,
    REASONABLE = 0
};

class LandmarkNode {
    int id;
    Landmark landmark;
public:
    LandmarkNode(Landmark &&landmark)
        : id(-1), landmark(std::move(landmark)) {
    }

    std::unordered_map<LandmarkNode *, EdgeType> parents;
    std::unordered_map<LandmarkNode *, EdgeType> children;

    int get_id() const {
        return id;
    }

    // TODO: Should possibly not be changeable
    void set_id(int new_id) {
        assert(id == -1 || new_id == id);
        id = new_id;
    }

    // TODO: Remove this function once the LM-graph is constant after creation.
    Landmark &get_landmark() {
        return landmark;
    }

    const Landmark &get_landmark() const {
        return landmark;
    }
};

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

    void remove_node_occurrences(LandmarkNode *node);

public:
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkGraph();

    // needed by both landmarkgraph-factories and non-landmarkgraph-factories
    const Nodes &get_nodes() const {
        return nodes;
    }
    // needed by both landmarkgraph-factories and non-landmarkgraph-factories
    int get_num_landmarks() const {
        return nodes.size();
    }
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    int get_num_disjunctive_landmarks() const {
        return num_disjunctive_landmarks;
    }
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    int get_num_conjunctive_landmarks() const {
        return num_conjunctive_landmarks;
    }
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    int get_num_edges() const;

    // only needed by non-landmarkgraph-factories
    LandmarkNode *get_node(int index) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkNode &get_simple_landmark(const FactPair &fact) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkNode &get_disjunctive_landmark(const FactPair &fact) const;

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there.  It is not needed by
       HMLandmarkFactory*/
    bool contains_simple_landmark(const FactPair &lm) const;
    /* Only used internally. */
    bool contains_disjunctive_landmark(const FactPair &lm) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there.  It is not needed by
       HMLandmarkFactory*/
    bool contains_overlapping_disjunctive_landmark(const std::set<FactPair> &lm) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    bool contains_identical_disjunctive_landmark(const std::set<FactPair> &lm) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there.  It is not needed by
       HMLandmarkFactory*/
    bool contains_landmark(const FactPair &fact) const;

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkNode &add_landmark(Landmark &&landmark);
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    void remove_node(LandmarkNode *node);
    void remove_node_if(
        const std::function<bool (const LandmarkNode &)> &remove_node_condition);

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    void set_landmark_ids();
};
}

#endif
