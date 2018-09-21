#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "evaluator.h"
#include "operator_id.h"
#include "per_state_information.h"
#include "task_proxy.h"

#include "algorithms/ordered_set.h"

#include <memory>
#include <vector>

class TaskProxy;

namespace options {
class OptionParser;
class Options;
}

class Heuristic : public Evaluator {
    struct HEntry {
        /* dirty is conceptually a bool, but Visual C++ does not support
           packing ints and bools together in a bitfield. */
        int h : 31;
        unsigned int dirty : 1;

        HEntry(int h, bool dirty)
            : h(h), dirty(dirty) {
        }
    };
    static_assert(sizeof(HEntry) == 4, "HEntry has unexpected size.");

    /*
      TODO: We might want to get rid of the preferred_operators
      attribute. It is currently only used by compute_result() and the
      methods it calls (compute_heuristic() directly, further methods
      indirectly), and we could e.g. change this by having
      compute_heuristic return an EvaluationResult object.

      If we do this, we should be mindful of the cost incurred by not
      being able to reuse the data structure from one iteration to the
      next, but this seems to be the only potential downside.
    */
    ordered_set::OrderedSet<OperatorID> preferred_operators;

protected:
    /*
      Cache for saving h values
      Before accessing this cache always make sure that the cache_evaluator_values
      flag is set to true - as soon as the cache is accessed it will create
      entries for all existing states
    */
    PerStateInformation<HEntry> heuristic_cache;
    bool cache_evaluator_values;

    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;

    enum {DEAD_END = -1, NO_VALUE = -2};

    // TODO: Call with State directly once all heuristics support it.
    virtual int compute_heuristic(const GlobalState &state) = 0;

    /*
      Usage note: Marking the same operator as preferred multiple times
      is OK -- it will only appear once in the list of preferred
      operators for this heuristic.
    */
    void set_preferred(const OperatorProxy &op);

    /* TODO: Make private and use State instead of GlobalState once all
       heuristics use the TaskProxy class. */
    State convert_global_state(const GlobalState &global_state) const;

public:
    explicit Heuristic(const options::Options &opts);
    virtual ~Heuristic() override;

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> & /*evals*/) override {
    }

    static void add_options_to_parser(options::OptionParser &parser);

    virtual EvaluationResult compute_result(
        EvaluationContext &eval_context) override;

    virtual bool does_cache_estimates() const override;
    virtual bool is_estimate_cached(const GlobalState &state) const override;
    virtual int get_cached_estimate(const GlobalState &state) const override;
};

#endif
