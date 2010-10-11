#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <set>

class Heuristic;
class Operator;
class State;

class Evaluator {
public:
    virtual ~Evaluator() {}

    virtual void evaluate(int g, bool preferred) = 0;
    virtual bool is_dead_end() const = 0;
    virtual bool dead_end_is_reliable() const = 0;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) = 0;
};

#endif
