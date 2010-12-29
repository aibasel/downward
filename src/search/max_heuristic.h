#ifndef MAX_HEURISTIC_H
#define MAX_HEURISTIC_H

#include "relaxation_heuristic.h"
#include <vector>
#include <ext/hash_set>

class Operator;
class State;

class Proposition;
class UnaryOperator;


class HSPMaxHeuristic : public RelaxationHeuristic {
    Proposition **reachable_queue_start;
    Proposition **reachable_queue_read_pos;
    Proposition **reachable_queue_write_pos;

    void setup_exploration_queue();
    void setup_exploration_queue_state(const State &state);
    void relaxed_exploration();
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    HSPMaxHeuristic(const HeuristicOptions &options);
    ~HSPMaxHeuristic();

    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
