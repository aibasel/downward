#ifndef IPC_MAX_HEURISTIC_H
#define IPC_MAX_HEURISTIC_H

#include "heuristic.h"

#include <vector>

class Options;

class IPCMaxHeuristic : public Heuristic {
    std::vector<Heuristic *> heuristics;

protected:
    virtual int compute_heuristic(const GlobalState &state) override;

public:
    explicit IPCMaxHeuristic(const Options &options);
    virtual ~IPCMaxHeuristic() = default;

    virtual bool reach_state(const GlobalState &parent_state,
                             const GlobalOperator &op,
                             const GlobalState &state) override;
};

#endif
