#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "./../heuristic.h"
#include "./../option_parser.h"
#include "./../state.h"
#include "./abstraction.h"

namespace cegar_heuristic {
class CegarHeuristic : public Heuristic {
    int refinements;
    Abstraction abstraction;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
