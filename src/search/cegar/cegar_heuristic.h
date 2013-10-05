#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "task.h"
#include "utils.h"
#include "../heuristic.h"
#include "../option_parser.h"

#include <unordered_set>

class State;

namespace cegar_heuristic {
class Abstraction;
class Task;

const int DEFAULT_STATES_OFFLINE = 10000;

enum GoalOrder {
    ORIGINAL,
    MIXED,
    CG_FORWARD,
    CG_BACKWARD,
    DOMAIN_SIZE_UP,
    DOMAIN_SIZE_DOWN,
    HADD_UP,
    HADD_DOWN
};

enum Decomposition {
    NONE,
    ALL_LANDMARKS,
    RANDOM_LANDMARKS,
    GOAL_FACTS
};

class CegarHeuristic : public Heuristic {
    const Options options;
    const bool search;
    const GoalOrder fact_order;
    std::vector<int> remaining_costs;
    Task original_task;
    std::vector<Task> tasks;
    std::vector<Abstraction *> abstractions;
    int num_states_offline;
    std::vector<double> avg_h_values;
    LandmarkGraph landmark_graph;

    LandmarkGraph get_landmark_graph() const;

    void mark_relevant_operators(std::vector<Operator> &operators, Fact fact) const;
    void add_operators(Task &task);

    // Compute the possible-before-set of facts that can be reached in the
    // delete-relaxation before last_fact is reached the first time.
    void get_possibly_before_facts(const Fact last_fact, unordered_set<int> *reached) const;

    void order_facts(vector<Fact> &facts) const;
    void get_fact_landmarks(std::vector<Fact> *facts) const;
    void generate_tasks(std::vector<Task> *tasks) const;
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
