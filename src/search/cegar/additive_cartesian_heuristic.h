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

class AdditiveCartesianHeuristic : public Heuristic {
    const Options options;
    std::vector<std::shared_ptr<Decomposition> > decompositions;
    const int max_states;
    std::unique_ptr<CountdownTimer> timer;
    std::vector<int> remaining_costs;
    std::vector<CartesianHeuristic> heuristics;
    int num_abstractions;
    int num_states;

    void reduce_remaining_costs(const std::vector<int> &needed_costs);
    std::shared_ptr<AbstractTask> get_remaining_costs_task(std::shared_ptr<AbstractTask> parent) const;
    bool may_build_another_abstraction();
    void build_abstractions(const Decomposition &decomposition);
    void print_statistics() const;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit AdditiveCartesianHeuristic(const Options &options);
    ~AdditiveCartesianHeuristic() = default;
};
}

#endif
