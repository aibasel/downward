#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "operator_cost.h"
#include "scalar_evaluator.h"
#include "task_proxy.h"

#include <memory>
#include <vector>

class GlobalOperator;
class GlobalState;
class OptionParser;
class Options;
class TaskProxy;

class Heuristic : public ScalarEvaluator {
    enum {NOT_INITIALIZED = -2};
    int heuristic;
    int evaluator_value; // usually equal to heuristic but can be different
    // if set with set_evaluator_value which is done if we use precalculated
    // estimates, eg. when re-opening a search node

    std::vector<const GlobalOperator *> preferred_operators;
protected:
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    OperatorCost cost_type;
    enum {DEAD_END = -1};
    virtual void initialize() {}
    // TODO: Call with State directly once all heuristics support it.
    virtual int compute_heuristic(const GlobalState &state) = 0;
    // Usage note: It's OK to set the same operator as preferred
    // multiple times -- it will still only appear in the list of
    // preferred operators for this heuristic once.
    // TODO: Make private once all heuristics use the TaskProxy class.
    void set_preferred(const GlobalOperator *op);
    void set_preferred(OperatorProxy op);
    // TODO: Remove once all heuristics use the TaskProxy class.
    int get_adjusted_cost(const GlobalOperator &op) const;
    // TODO: Make private once all heuristics use the TaskProxy class.
    State convert_global_state(const GlobalState &global_state) const;
public:
    Heuristic(const Options &options);
    virtual ~Heuristic();

    void evaluate(const GlobalState &state);
    bool is_dead_end() const;
    int get_heuristic();
    // changed to virtual, so HeuristicProxy can delegate this:
    virtual void get_preferred_operators(std::vector<const GlobalOperator *> &result);
    virtual bool dead_ends_are_reliable() const {return true; }
    virtual bool reach_state(const GlobalState &parent_state, const GlobalOperator &op,
                             const GlobalState &state);

    // virtual methods inherited from Evaluator and ScalarEvaluator:
    virtual int get_value() const;
    virtual void evaluate(int g, bool preferred);
    virtual bool dead_end_is_reliable() const;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) {hset.insert(this); }

    void set_evaluator_value(int val);
    OperatorCost get_cost_type() const {return cost_type; }

    static void add_options_to_parser(OptionParser &parser);
    static Options default_options();
};

std::shared_ptr<AbstractTask> get_task_from_options(const Options &opts);

#endif
