#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "../heuristic.h"

#include "split_tree.h"

class GlobalState;
class Options;

namespace cegar {
class CartesianHeuristic : public Heuristic {
    // TODO: We only save this to keep the task from being deleted. Avoid this.
    const std::shared_ptr<AbstractTask> abstract_task;
    // TODO: Rethink ownership and make sure SplitTree is deleted.
    const SplitTree split_tree;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CartesianHeuristic(std::shared_ptr<AbstractTask> abstract_task,
                                const Options &options);
    ~CartesianHeuristic();
};
}

#endif
