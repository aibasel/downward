#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "split_tree.h"

#include "../heuristic.h"

#include <memory>

class GlobalState;
class Options;

namespace cegar {
class CartesianHeuristic : public Heuristic {
    // TODO: We only save this to keep the task from being deleted. Avoid this.
    const std::shared_ptr<AbstractTask> abstract_task;
    // TODO: Store as unique_ptr.
    const SplitTree split_tree;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    // TODO: Move parameters into options once they support smart pointers.
    explicit CartesianHeuristic(std::shared_ptr<AbstractTask> abstract_task,
                                const Options &options);
    ~CartesianHeuristic();
};
}

#endif
