#ifndef LANDMARKS_LANDMARK_GRAPH_H
#define LANDMARKS_LANDMARK_GRAPH_H

#include "landmark.h"

#include "../task_proxy.h"

#include "../utils/hash.h"

#include <cassert>
#include <set>
#include <unordered_map>
#include <vector>

namespace landmarks {
enum class OrderingType {
    /*
      NOTE: The code relies on the fact that larger numbers are stronger in the
      sense that, e.g., every greedy-necessary ordering is also natural and
      reasonable. (It is a sad fact of terminology that necessary is indeed a
      special case of greedy-necessary, i.e., every necessary ordering is
      greedy-necessary, but not vice versa.)
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
    explicit LandmarkNode(Landmark &&landmark)
        : id(-1), landmark(std::move(landmark)) {
    }

    bool operator==(const LandmarkNode &other) const {
        return this == &other;
    }

    std::unordered_map<LandmarkNode *, OrderingType> parents;
    std::unordered_map<LandmarkNode *, OrderingType> children;

    int get_id() const {
        return id;
    }

    // TODO: Should possibly not be changeable.
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
    /* TODO: Make this a vector<LandmarkNode> once landmark graphs remain
       static. (issue993) */
    std::vector<std::unique_ptr<LandmarkNode>> nodes;

    int num_conjunctive_landmarks;
    int num_disjunctive_landmarks;

    utils::HashMap<FactPair, LandmarkNode *> atomic_landmarks_to_nodes;
    utils::HashMap<FactPair, LandmarkNode *> disjunctive_landmarks_to_nodes;

    void remove_node_occurrences(LandmarkNode *node);
    LandmarkNode *add_node(Landmark &&landmark);

public:
    // TODO: Remove once landmark graphs remain static. (issue993)
    using iterator = std::vector<std::unique_ptr<LandmarkNode>>::iterator;
    iterator begin() {
        return nodes.begin();
    }
    iterator end() {
        return nodes.end();
    }

    using const_iterator =
        std::vector<std::unique_ptr<LandmarkNode>>::const_iterator;
    const const_iterator begin() const {
        return nodes.cbegin();
    }
    const const_iterator end() const {
        return nodes.cend();
    }

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkGraph();

    // Needed by both landmark graph factories and non-landmark-graph factories.
    int get_num_landmarks() const {
        return static_cast<int>(nodes.size());
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
    int get_num_orderings() const;

    // Only needed by non-landmarkgraph-factories.
    const LandmarkNode *get_node(int index) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkNode &get_atomic_landmark_node(const FactPair &atom) const;
    /* This is needed only by landmark graph factories and will disappear
     get_num_landmarks  when moving landmark graph creation there. */
    LandmarkNode &get_disjunctive_landmark_node(const FactPair &atom) const;

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. It is not needed by
       HMLandmarkFactory. */
    bool contains_atomic_landmark(const FactPair &atom) const;
    // Only used internally.
    bool contains_disjunctive_landmark(const FactPair &atom) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. It is not needed by
       HMLandmarkFactory. */
    bool contains_overlapping_disjunctive_landmark(
        const utils::HashSet<FactPair> &atoms) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    bool contains_superset_disjunctive_landmark(
        const utils::HashSet<FactPair> &atoms) const;
    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. It is not needed by
       HMLandmarkFactory. */
    bool contains_landmark(const FactPair &atom) const;

    /* This is needed only by landmark graph factories and will disappear
       when moving landmark graph creation there. */
    LandmarkNode &add_landmark(Landmark &&landmark_to_add);
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
