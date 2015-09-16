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
    std::string description;
    bool initialized;

    /*
      TODO: We might want to get rid of the preferred_operators
      attribute. It is currently only used by compute_result() and the
      methods it calls (compute_heuristic() directly, further methods
      indirectly), and we could e.g. change this by having
      compute_heuristic return an EvaluationResult object.

      If we do this, we should be mindful of the cost incurred by not
      being able to reuse a vector from one iteration to the next, but
      this seems to be the only potential downside.
    */
    std::vector<const GlobalOperator *> preferred_operators;
protected:
    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
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
    virtual ~Heuristic() override;

    virtual bool reach_state(
        const GlobalState &parent_state, const GlobalOperator &op,
        const GlobalState &state);

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        hset.insert(this);
    }

    OperatorCost get_cost_type() const {return cost_type; }

    static void add_options_to_parser(OptionParser &parser);
    static Options default_options();

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    std::string get_description() const;
};

#endif
