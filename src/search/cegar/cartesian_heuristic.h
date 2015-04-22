#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "split_tree.h"

#include "../heuristic.h"

class GlobalState;
class Options;

namespace cegar {
class CartesianHeuristic : public Heuristic {
    // TODO: Store as unique_ptr.
    const SplitTree split_tree;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CartesianHeuristic(const Options &options);
    ~CartesianHeuristic() = default;
};
}

#endif
