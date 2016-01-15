#ifndef PDBS_ZERO_ONE_PDBS_HEURISTIC_H
#define PDBS_ZERO_ONE_PDBS_HEURISTIC_H

#include "zero_one_pdbs.h"

#include "../heuristic.h"

namespace pdbs {
class PatternDatabase;

class ZeroOnePDBsHeuristic : public Heuristic {
    ZeroOnePDBs zero_one_pdbs;
protected:
    virtual int compute_heuristic(const GlobalState &global_state);
    /* TODO: we want to get rid of compute_heuristic(const GlobalState &state)
       and change the interface to only use State objects. While we are doing
       this, the following method already allows to get the heuristic value
       for a State object. */
    int compute_heuristic(const State &state) const;
public:
    ZeroOnePDBsHeuristic(const options::Options &opts);
    virtual ~ZeroOnePDBsHeuristic() = default;
};
}

#endif
