#ifndef FD_HEURISTIC_H
#define FD_HEURISTIC_H

#include "heuristic.h"

#include <vector>
#include <math.h>

class Abstraction;

class FinkbeinerDraegerHeuristic : public Heuristic {
    std::vector<Abstraction *> abstractions;
    void verify_no_axioms_no_cond_effects() const;
    Abstraction *build_abstraction(bool is_first = true);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    FinkbeinerDraegerHeuristic();
    ~FinkbeinerDraegerHeuristic();
};

#endif
