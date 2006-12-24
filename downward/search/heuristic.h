#ifndef HEURISTIC_H
#define HEURISTIC_H

#include <map>
#include <vector>

class Operator;
class State;

class Heuristic {
    enum {NOT_INITIALIZED = -2};
    int heuristic;
    std::vector<const Operator *> preferred_operators;

    struct EvaluationInfo {
	EvaluationInfo() {heuristic = NOT_INITIALIZED;}
	EvaluationInfo(int heur, const std::vector<const Operator *> &pref)
	    : heuristic(heur), preferred_operators(pref) {}
	int heuristic;
	std::vector<const Operator *> preferred_operators;
    };

    bool use_cache;
    std::map<State, EvaluationInfo> state_cache;
protected:
    enum {DEAD_END = -1};
    virtual void initialize() {}
    virtual int compute_heuristic(const State &state) = 0;
    void set_preferred(const Operator *op);
public:
    Heuristic(bool use_cache=false);
    virtual ~Heuristic();

    void evaluate(const State &state);
    bool is_dead_end();
    int get_heuristic();
    void get_preferred_operators(std::vector<const Operator *> &result);
    virtual bool dead_ends_are_reliable() {return true;}
};

#endif
