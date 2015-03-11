#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "operator_cost.h"
#include "scalar_evaluator.h"
#include "task_proxy.h"

#include <vector>

class GlobalOperator;
class GlobalState;
class OptionParser;
class Options;

class Heuristic : public ScalarEvaluator {
    bool initialized;

    std::vector<const GlobalOperator *> preferred_operators;
protected:
    TaskProxy *task;
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
    int get_adjusted_cost(const OperatorProxy &op) const;
    // TODO: Make private once all heuristics use the TaskProxy class.
    State convert_global_state(const GlobalState &global_state) const;

public:
    Heuristic(const Options &options);
    virtual ~Heuristic() override;

    // changed to virtual, so HeuristicProxy can delegate this:
    virtual void get_preferred_operators(
        std::vector<const GlobalOperator *> &result);
    virtual bool dead_ends_are_reliable() const {
        return true;
    }
    virtual bool reach_state(
        const GlobalState &parent_state, const GlobalOperator &op,
        const GlobalState &state);

    // virtual methods inherited from ScalarEvaluator:
    virtual bool dead_end_is_reliable() const override;
    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        hset.insert(this);
    }

    OperatorCost get_cost_type() const {return cost_type; }

    static void add_options_to_parser(OptionParser &parser);
    static Options default_options();

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;
};

#endif
