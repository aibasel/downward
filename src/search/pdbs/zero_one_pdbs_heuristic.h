#ifndef PDBS_ZERO_ONE_PDBS_HEURISTIC_H
#define PDBS_ZERO_ONE_PDBS_HEURISTIC_H

#include "zero_one_pdbs.h"

#include "../heuristic.h"

#include <memory>
#include <vector>

class PatternDatabase;

class ZeroOnePDBsHeuristic : public Heuristic {
    ZeroOnePDBs zero_one_pdbs;
protected:
    virtual int compute_heuristic(const GlobalState &global_state);
    int compute_heuristic(const State &state);
public:
    ZeroOnePDBsHeuristic(const Options &opts);
    virtual ~ZeroOnePDBsHeuristic();
};

#endif
