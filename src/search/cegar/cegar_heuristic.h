#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

#include <memory>
#include <vector>

class CountdownTimer;
class FactProxy;
class GlobalState;

namespace cegar {
class CartesianHeuristic;

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
    int max_states;
    std::unique_ptr<CountdownTimer> timer;
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
    ~CegarHeuristic() = default;
};
}

#endif
