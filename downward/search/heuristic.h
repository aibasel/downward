#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "scalar_evaluator.h"

#include <map>
#include <vector>

class Operator;
class State;

class Heuristic : public ScalarEvaluator {
    enum {NOT_INITIALIZED = -2};
    int heuristic;
    int evaluator_value; // usually equal to heuristic but can be different
	// if set with set_evaluator_value which is done if we use precalculated
	// estimates, eg. when re-opening a search node

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
    bool is_dead_end() const;
    int get_heuristic();
    // changed to virtual, so HeuristicProxy can delegate this:
    virtual void get_preferred_operators(std::vector<const Operator *> &result);
    virtual bool dead_ends_are_reliable() const {return true;}
    virtual bool reach_state(const State& parent_state, const Operator& op,
        		const State& state);

	// for abstract parent ScalarEvaluator
	int get_value() const;
	void evaluate(int g, bool preferred);
	bool dead_end_is_reliable() const;
	void set_evaluator_value(int val);
};

#endif
