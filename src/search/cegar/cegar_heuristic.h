#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "abstraction.h"
#include "../heuristic.h"
#include "../option_parser.h"
#include "../state.h"

namespace cegar_heuristic {
const int COST_UPDATES = 3;

class CegarHeuristic : public Heuristic {
    int max_states;
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
