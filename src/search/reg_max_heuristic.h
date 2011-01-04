#ifndef REG_MAX_HEURISTIC_H
#define REG_MAX_HEURISTIC_H

#include "heuristic.h"

#include <string>
#include <vector>

class RegMaxHeuristic : public Heuristic {
private:
    std::vector<Heuristic *> evaluators;
    int value;
    bool dead_end;
    bool dead_end_reliable;

protected:
    virtual int compute_heuristic(const State &state);

public:
    RegMaxHeuristic(const HeuristicOptions &options, const std::vector<Heuristic *> &evals);
    ~RegMaxHeuristic();
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
