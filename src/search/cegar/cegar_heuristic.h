#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

class State;

namespace cegar_heuristic {

class Abstraction;

const int DEFAULT_STATES_OFFLINE = 10000;

class CegarHeuristic : public Heuristic {
    const Options options;
    // How many times should the goal distances be updated? A higher number
    // makes for a better heuristic during the A* searches, but adds additional
    // runtime.
    const int h_updates;
    const bool search;
    Abstraction *abstraction;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    explicit CegarHeuristic(const Options &options);
    ~CegarHeuristic();
};
}

#endif
