#ifndef PARTIAL_RELAXATION_HEURISTIC_H
#define PARTIAL_RELAXATION_HEURISTIC_H

#include "heuristic.h"

class LowLevelSearchEngine;
class PartialRelaxation;

class PartialRelaxationHeuristic : public Heuristic {
    PartialRelaxation *relaxation;
    Heuristic *low_level_heuristic;
    LowLevelSearchEngine *low_level_search_engine;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    PartialRelaxationHeuristic();
    ~PartialRelaxationHeuristic();
};

#endif
