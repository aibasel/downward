#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "landmark_task.h"
#include "utils.h"

#include "../heuristic.h"
#include "../option_parser.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

class GlobalState;

namespace cegar {
class Abstraction;
class LandmarkTask;

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
    LANDMARKS_AND_GOALS,
    LANDMARKS_AND_GOALS_AND_NONE
};

class CegarHeuristic : public Heuristic {
    const Options options;
    const bool search;
    int max_states;
    const int max_time;
    const GoalOrder fact_order;
    std::vector<int> remaining_costs;
    LandmarkTask original_task;
    std::vector<LandmarkTask> tasks;
    std::vector<Abstraction *> abstractions;
    int num_states;
    LandmarkGraph landmark_graph;
    int *temp_state_buffer;

    LandmarkGraph get_landmark_graph() const;
    VariableToValues get_prev_landmarks(FactProxy fact) const;

    void add_operators(LandmarkTask &task);

    // Compute the possibly-before-set of facts that can be reached in the
    // delete-relaxation before last_fact is reached the first time.
    void get_possibly_before_facts(const Fact last_fact, std::unordered_set<int> *reached) const;

    void order_facts(std::vector<Fact> &facts) const;
    void get_fact_landmarks(std::vector<Fact> *facts) const;
    void get_facts(std::vector<Fact> &facts, Decomposition decomposition) const;
    void install_task(LandmarkTask &task) const;
    void build_abstractions(Decomposition decomposition);

protected:
    virtual void print_statistics();
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);

public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
