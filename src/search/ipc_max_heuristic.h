#ifndef IPC_MAX_HEURISTIC_H
#define IPC_MAX_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class Options;

class IPCMaxHeuristic : public Heuristic {
    std::vector<Heuristic *> evaluators;
    int value;
    bool dead_end;
    bool dead_end_reliable;

protected:
    virtual int compute_heuristic(const State &state);

public:
    IPCMaxHeuristic(const Options &options);
    ~IPCMaxHeuristic();
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);
};

#endif
