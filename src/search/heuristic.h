#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "scalar_evaluator.h"
#include "option_parser.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class Operator;
class State;

enum OperatorCost {NORMAL = 0, ONE = 1, PLUSONE = 2, MAX_OPERATOR_COST};

struct HeuristicOptions {
    int cost_type;

    HeuristicOptions();

    void add_option_to_parser(NamedOptionParser &option_parser);
};

class Heuristic : public ScalarEvaluator {
private:
    enum {NOT_INITIALIZED = -2};
    OperatorCost cost_type;
    int heuristic;
    int evaluator_value; // usually equal to heuristic but can be different
    // if set with set_evaluator_value which is done if we use precalculated
    // estimates, eg. when re-opening a search node

    std::vector<const Operator *> preferred_operators;

protected:
    enum {DEAD_END = -1};
    virtual void initialize() {}
    virtual int compute_heuristic(const State &state) = 0;
    // Usage note: It's OK to set the same operator as preferred
    // multiple times -- it will still only appear in the list of
    // preferred operators for this heuristic once.
    void set_preferred(const Operator *op);
    int get_adjusted_cost(int cost) const;
public:
    Heuristic(HeuristicOptions &options);
    virtual ~Heuristic();

    void evaluate(const State &state);
    bool is_dead_end() const;
    int get_heuristic();
    // changed to virtual, so HeuristicProxy can delegate this:
    virtual void get_preferred_operators(std::vector<const Operator *> &result);
    virtual bool dead_ends_are_reliable() const {return true; }
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);

    // for abstract parent ScalarEvaluator
    int get_value() const;
    void evaluate(int g, bool preferred);
    bool dead_end_is_reliable() const;
    void set_evaluator_value(int val);
    void get_involved_heuristics(std::set<Heuristic *> &hset) {hset.insert(this); }
    virtual void reset() {}
};

#endif
