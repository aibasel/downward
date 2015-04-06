#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "domain_abstracted_task.h"
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
class CartesianHeuristic;
class DomainAbstractedTask;

enum class TaskOrder {
    ORIGINAL,
    MIXED,
    HADD_UP,
    HADD_DOWN
};

enum class Decomposition {
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
    const double max_time;
    const TaskOrder task_order;
    std::vector<int> remaining_costs;
    std::vector<CartesianHeuristic> heuristics;
    int num_states;
    const std::shared_ptr<LandmarkGraph> landmark_graph;

    void order_facts(std::vector<FactProxy> &facts) const;
    std::vector<FactProxy> get_facts(Decomposition decomposition) const;
    void build_abstractions(Decomposition decomposition);
    void print_statistics();

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
