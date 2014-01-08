#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "task.h"
#include "utils.h"
#include "../heuristic.h"
#include "../option_parser.h"

#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <vector>

class State;

namespace cegar_heuristic {
class Abstraction;
class Task;

enum GoalOrder {
    ORIGINAL,
    MIXED,
    HADD_UP,
    HADD_DOWN
};

enum Decomposition {
    NONE,
    LANDMARKS,
    GOALS,
    GOAL_LEAVES,
    LANDMARKS_AND_GOALS
};

class CegarHeuristic : public Heuristic {
    const Options options;
    const bool search;
    int max_states;
    const int max_time;
    const GoalOrder fact_order;
    std::vector<int> remaining_costs;
    Task original_task;
    std::vector<Task> tasks;
    std::vector<Abstraction *> abstractions;
    int num_states;
    std::vector<double> avg_h_values;
    LandmarkGraph landmark_graph;
    state_var_t *temp_state_buffer;

    LandmarkGraph get_landmark_graph() const;
    void get_prev_landmarks(Fact fact, std::tr1::unordered_map<int, std::tr1::unordered_set<int> > *groups) const;

    void mark_relevant_operators(std::vector<Operator> &operators, Fact fact) const;
    void add_operators(Task &task);

    // Compute the possible-before-set of facts that can be reached in the
    // delete-relaxation before last_fact is reached the first time.
    void get_possibly_before_facts(const Fact last_fact, std::tr1::unordered_set<int> *reached) const;

    void order_facts(std::vector<Fact> &facts) const;
    void get_fact_landmarks(std::vector<Fact> *facts) const;
    void get_facts(std::vector<Fact> &facts, Decomposition decomposition) const;
    void install_task(Task &task) const;
    void build_abstractions(Decomposition decomposition);

protected:
    virtual void print_statistics();
    virtual void initialize();
    virtual int compute_heuristic(const State &state);

public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
