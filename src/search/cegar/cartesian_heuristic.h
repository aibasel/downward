#ifndef CEGAR_CARTESIAN_HEURISTIC_H
#define CEGAR_CARTESIAN_HEURISTIC_H

#include "split_tree.h"

#include "../heuristic.h"

class GlobalState;
class Options;

namespace CEGAR {
class CartesianHeuristic : public Heuristic {
    const SplitTree split_tree;

protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);

public:
    explicit CartesianHeuristic(const Options &options, SplitTree &&split_tree);
    ~CartesianHeuristic() = default;
};
}

#endif
