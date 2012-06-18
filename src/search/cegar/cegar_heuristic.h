#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "abstraction.h"
#include "../heuristic.h"
#include "../option_parser.h"
#include "../state.h"

namespace cegar_heuristic {

enum PickStrategy {
    FIRST,
    RANDOM
};

class CegarHeuristic : public Heuristic {
    int max_states;
    const PickStrategy pick_strategy;
    Abstraction abstraction;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
