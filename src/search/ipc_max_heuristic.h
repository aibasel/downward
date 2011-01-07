#ifndef REG_MAX_HEURISTIC_H
#define REG_MAX_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class IPCMaxHeuristic : public Heuristic {
    std::vector<Heuristic *> evaluators;
    int value;
    bool dead_end;
    bool dead_end_reliable;

protected:
    virtual int compute_heuristic(const State &state);

public:
    IPCMaxHeuristic(const HeuristicOptions &options, const std::vector<Heuristic *> &evals);
    ~IPCMaxHeuristic();
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);
};

#endif
