#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "../heuristic.h"

#include "split_tree.h"

class GlobalState;
class Options;

namespace cegar {
class CartesianHeuristic : public Heuristic {
    // TODO: Rethink ownership and make sure SplitTree is deleted.
    const SplitTree split_tree;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CartesianHeuristic(const Options &options);
    ~CartesianHeuristic();
};
}

#endif
