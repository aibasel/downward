#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

#include <memory>
#include <vector>

class CountdownTimer;
class Decomposition;
class GlobalState;

namespace cegar {
class CartesianHeuristic;

class CegarHeuristic : public Heuristic {
    const Options options;
    std::vector<Decomposition *> decompositions;
    int max_states;
    std::unique_ptr<CountdownTimer> timer;
    std::vector<int> remaining_costs;
    std::vector<CartesianHeuristic> heuristics;
    int num_states;

    std::shared_ptr<AbstractTask> get_remaining_costs_task(std::shared_ptr<AbstractTask> parent) const;
    void build_abstractions(const Decomposition &decomposition);
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
