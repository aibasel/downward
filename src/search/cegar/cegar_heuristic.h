#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "landmark_task.h"
#include "utils.h"

#include "../heuristic.h"
#include "../option_parser.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FactProxy;
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
    // TODO: Save TaskProxys directly? Then we would have to store AbstractTasks
    //       as smart pointers in proxy classes.
    std::vector<std::shared_ptr<AbstractTask>> tasks;
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

    void order_facts(std::vector<FactProxy> &facts) const;
    std::vector<FactProxy> get_fact_landmarks() const;
    std::vector<FactProxy> get_facts(Decomposition decomposition) const;
    void build_abstractions(Decomposition decomposition);

protected:
    virtual void print_statistics();
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
