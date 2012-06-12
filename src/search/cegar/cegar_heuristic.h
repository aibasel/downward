#ifndef CEGAR_CEGAR_HEURISTIC_H
#define CEGAR_CEGAR_HEURISTIC_H

#include <ext/slist>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>

#include "../heuristic.h"
#include "../operator.h"
#include "./abstract_state.h"
#include "./abstraction.h"

#include "gtest/gtest_prod.h"

namespace cegar_heuristic {

int REFINEMENTS = 10;

class CegarHeuristic : public Heuristic {
    int min_operator_cost;
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
