#ifndef CEGAR_CEGAR_SUM_HEURISTIC_H
#define CEGAR_CEGAR_SUM_HEURISTIC_H

#include "../heuristic.h"
#include "../option_parser.h"

class State;

namespace cegar_heuristic {
class Abstraction;

const int DEFAULT_STATES_OFFLINE = 10000;

enum GoalOrder {
    ORIGINAL,
    MIXED,
    CG_FORWARD,
    CG_BACKWARD,
    DOMAIN_SIZE_UP,
    DOMAIN_SIZE_DOWN
};

class CegarSumHeuristic : public Heuristic {
    const Options options;
    const bool search;
    const GoalOrder goal_order;
    std::vector<Abstraction *> abstractions;
    std::vector<double> avg_h_values;
protected:
    virtual void print_statistics();
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    explicit CegarSumHeuristic(const Options &options);
    ~CegarSumHeuristic();
};
}

#endif
