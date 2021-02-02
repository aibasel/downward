#ifndef HEURISTICS_CEA_HEURISTIC_H
#define HEURISTICS_CEA_HEURISTIC_H

#include "domain_transition_graph.h"

#include "../heuristic.h"

#include "../algorithms/priority_queues.h"

#include <vector>

class State;

namespace cea_heuristic {
struct LocalProblem;
struct LocalProblemNode;
struct LocalTransition;

class ContextEnhancedAdditiveHeuristic : public Heuristic {
    std::vector<std::unique_ptr<domain_transition_graph::DomainTransitionGraph>> transition_graphs;
    std::vector<LocalProblem *> local_problems;
    std::vector<std::vector<LocalProblem *>> local_problem_index;
    LocalProblem *goal_problem;
    LocalProblemNode *goal_node;
    int min_action_cost;

    priority_queues::AdaptiveQueue<LocalProblemNode *> node_queue;

    LocalProblem *get_local_problem(int var_no, int value);
    LocalProblem *build_problem_for_variable(int var_no) const;
    LocalProblem *build_problem_for_goal() const;

    int get_priority(LocalProblemNode *node) const;
    void initialize_heap();
    void add_to_heap(LocalProblemNode *node);

    bool is_local_problem_set_up(const LocalProblem *problem) const;
    void set_up_local_problem(LocalProblem *problem, int base_priority,
                              int start_value, const State &state);

    void try_to_fire_transition(LocalTransition *trans);
    void expand_node(LocalProblemNode *node);
    void expand_transition(LocalTransition *trans, const State &state);

    int compute_costs(const State &state);
    void mark_helpful_transitions(
        LocalProblem *problem, LocalProblemNode *node, const State &state);
    // Clears "reached_by" of visited nodes as a side effect to avoid
    // recursing to the same node again.
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit ContextEnhancedAdditiveHeuristic(const options::Options &opts);
    ~ContextEnhancedAdditiveHeuristic();
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
