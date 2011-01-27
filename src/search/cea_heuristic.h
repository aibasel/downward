#ifndef CEA_HEURISTIC_H
#define CEA_HEURISTIC_H

#include <string>
#include <vector>

#include "heuristic.h"
#include "priority_queue.h"

class ValueTransitionLabel;
class State;

class LocalProblem;
class LocalProblemNode;
class ContextEnhancedAdditiveHeuristic;

// TODO: Fix friend statements and access qualifiers.

class LocalTransition {
    friend class ContextEnhancedAdditiveHeuristic;
    friend class LocalProblem;
    friend class LocalProblemNode;

    LocalProblemNode *source;
    LocalProblemNode *target;
    const ValueTransitionLabel *label;
    int action_cost;

    int target_cost;
    int unreached_conditions;

    LocalTransition(LocalProblemNode *source_, LocalProblemNode *target_,
                    const ValueTransitionLabel *label_, int action_cost_);

    void on_source_expanded(const State &state);
    void on_condition_reached(int cost);
    void try_to_fire();
};


class LocalProblemNode {
    friend class ContextEnhancedAdditiveHeuristic;
    friend class LocalProblem;
    friend class LocalTransition;

    // Static attributes (fixed upon initialization).
    LocalProblem *owner;
    std::vector<LocalTransition> outgoing_transitions;

    // Dynamic attributes (modified during heuristic computation).
    int cost;
    inline int priority() const;
    // Nodes have both a "cost" and a "priority", which are related.
    // The cost is an estimate of how expensive it is to reach this
    // node. The "priority" is the lowest cost value in the overall
    // cost computation for which this node will be important. It is
    // essentially the sum of the cost and a local-problem-specific
    // "base priority", which depends on where this local problem is
    // needed for the overall computation.

    bool expanded;
    std::vector<short> children_state;

    LocalTransition *reached_by;
    // Before a node is expanded, reached_by is the "current best"
    // transition leading to this node. After a node is expanded, the
    // reached_by value of the parent is copied (unless the parent is
    // the initial node), so that reached_by is the *first* transition
    // on the optimal path to this node. This is useful for preferred
    // operators. (The two attributes used to be separate, but this
    // was a bit wasteful.)

    std::vector<LocalTransition *> waiting_list;

    LocalProblemNode(LocalProblem *owner, int children_state_size);
    void add_to_waiting_list(LocalTransition *transition);
    void on_expand();

    void mark_helpful_transitions(const State &state);
    // Clears "reached_by" of visited nodes as a side effect to avoid
    // recursing to the same node again.
};


class LocalProblem {
    friend class ContextEnhancedAdditiveHeuristic;
    friend class LocalProblemNode;
    friend class LocalTransition;

    int base_priority;

    std::vector<LocalProblemNode> nodes;

    std::vector<int> *causal_graph_parents;

    void build_nodes_for_variable(int var_no);
    void build_nodes_for_goal();
    inline bool is_initialized() const;
public:
    LocalProblem(int var_no = -1);
    void initialize(int base_priority, int start_value, const State &state);
};


class ContextEnhancedAdditiveHeuristic : public Heuristic {
    friend class LocalProblem;
    friend class LocalProblemNode;
    friend class LocalTransition;

    std::vector<LocalProblem *> local_problems;
    std::vector<std::vector<LocalProblem *> > local_problem_index;
    LocalProblem *goal_problem;
    LocalProblemNode *goal_node;

    AdaptiveQueue<LocalProblemNode *> node_queue;

    int compute_costs(const State &state);
    void initialize_heap();
    void add_to_heap(LocalProblemNode *node);
    inline LocalProblem *get_local_problem(int var_no, int value);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    ContextEnhancedAdditiveHeuristic(const HeuristicOptions &options);
    ~ContextEnhancedAdditiveHeuristic();
    virtual bool dead_ends_are_reliable() const {return false; }
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};


inline int LocalProblemNode::priority() const {
    return cost + owner->base_priority;
}

inline bool LocalProblem::is_initialized() const {
    return base_priority != -1;
}

inline LocalProblem *ContextEnhancedAdditiveHeuristic::get_local_problem(
    int var_no, int value) {
    LocalProblem *result = local_problem_index[var_no][value];
    if (!result) {
        result = new LocalProblem(var_no);
        local_problem_index[var_no][value] = result;
        local_problems.push_back(result);
    }
    return result;
}

#endif
