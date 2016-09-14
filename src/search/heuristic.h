#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "per_state_information.h"
#include "scalar_evaluator.h"
#include "task_proxy.h"

#include <memory>
#include <vector>

class GlobalOperator;
class GlobalState;
class TaskProxy;

namespace options {
class OptionParser;
class Options;
}

class Heuristic : public ScalarEvaluator {
    struct HEntry {
        int h : 31;
        bool dirty : 1;
        HEntry(int h, bool dirty) : h(h), dirty(dirty) {}
    };

    std::string description;

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

    void set_preferred(const GlobalOperator *op);

protected:
    /*
      Cache for saving h values
      Before accessing this cache always make sure that the cache_h_values
      flag is set to true - as soon as the cache is accessed it will create
      entries for all existing states
    */
    PerStateInformation<HEntry> heuristic_cache;
    bool cache_h_values;

    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

    enum {DEAD_END = -1, NO_VALUE = -2};

    // TODO: Call with State directly once all heuristics support it.
    virtual int compute_heuristic(const GlobalState &state) = 0;

    // Usage note: It's OK to set the same operator as preferred
    // multiple times -- it will still only appear in the list of
    // preferred operators for this heuristic once.
    void set_preferred(const OperatorProxy &op);

    /* TODO: Make private and use State instead of GlobalState once all
       heuristics use the TaskProxy class. */
    State convert_global_state(const GlobalState &global_state) const;

public:
    explicit Heuristic(const options::Options &options);
    virtual ~Heuristic() override;

    virtual void notify_initial_state(const GlobalState & /*initial_state*/) {
    }

    virtual bool notify_state_transition(
        const GlobalState &parent_state, const GlobalOperator &op,
        const GlobalState &state);

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        hset.insert(this);
    }

    static void add_options_to_parser(options::OptionParser &parser);
    static options::Options default_options();

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    std::string get_description() const;
    bool is_h_dirty(GlobalState &state) {
        return heuristic_cache[state].dirty;
    }
};

#endif
