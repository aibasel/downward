#include "heuristic.h"

#include "global_operator.h"
#include "option_parser.h"
#include "operator_cost.h"
#include "root_task.h"
#include "task_proxy.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

Heuristic::Heuristic(const Options &opts)
    : task(new TaskProxy(opts.get<AbstractTask *>("task"))),
      cost_type(OperatorCost(opts.get_enum("cost_type"))) {
    heuristic = NOT_INITIALIZED;
}

Heuristic::~Heuristic() {
    delete task;
}

void Heuristic::set_preferred(const GlobalOperator *op) {
    if (!op->is_marked()) {
        op->mark();
        preferred_operators.push_back(op);
    }
}

void Heuristic::set_preferred(OperatorProxy op) {
    set_preferred(op.get_global_operator());
}

void Heuristic::evaluate(const GlobalState &state) {
    if (heuristic == NOT_INITIALIZED)
        initialize();
    preferred_operators.clear();
    heuristic = compute_heuristic(state);
    for (size_t i = 0; i < preferred_operators.size(); ++i)
        preferred_operators[i]->unmark();
    assert(heuristic == DEAD_END || heuristic >= 0);

    if (heuristic == DEAD_END) {
        // It is ok to have preferred operators in dead-end states.
        // This allows a heuristic to mark preferred operators on-the-fly,
        // selecting the first ones before it is clear that all goals
        // can be reached.
        preferred_operators.clear();
    }

#ifndef NDEBUG
    if (heuristic != DEAD_END) {
        for (size_t i = 0; i < preferred_operators.size(); ++i)
            assert(preferred_operators[i]->is_applicable(state));
    }
#endif
    evaluator_value = heuristic;
}

bool Heuristic::is_dead_end() const {
    return evaluator_value == DEAD_END;
}

int Heuristic::get_heuristic() {
    // The -1 value for dead ends is an implementation detail which is
    // not supposed to leak. Thus, calling this for dead ends is an
    // error. Call "is_dead_end()" first.

    /*
      TODO: I've commented the assertion out for now because there is
      currently code that calls get_heuristic for dead ends. For
      example, if we use alternation with h^FF and h^cea and have an
      instance where the initial state has infinite h^cea value, we
      should expand this state since h^cea is unreliable. The search
      progress class will then want to print the h^cea value of the
      initial state since this is the "best know h^cea state" so far.

      However, we should clean up the code again so that the assertion
      is valid or rethink the interface so that we don't need it.
     */

    // assert(heuristic >= 0);
    if (heuristic == DEAD_END)
        return numeric_limits<int>::max();
    return heuristic;
}

void Heuristic::get_preferred_operators(std::vector<const GlobalOperator *> &result) {
    assert(heuristic >= 0);
    result.insert(result.end(),
                  preferred_operators.begin(),
                  preferred_operators.end());
}

bool Heuristic::reach_state(const GlobalState & /*parent_state*/,
                            const GlobalOperator & /*op*/, const GlobalState & /*state*/) {
    return false;
}

int Heuristic::get_value() const {
    return evaluator_value;
}

void Heuristic::evaluate(int, bool) {
    return;
    // if this is called, evaluate(const GlobalState &state) or set_evaluator_value(int val)
    // should already have been called
}

bool Heuristic::dead_end_is_reliable() const {
    return dead_ends_are_reliable();
}

void Heuristic::set_evaluator_value(int val) {
    evaluator_value = val;
}

int Heuristic::get_adjusted_cost(const GlobalOperator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

int Heuristic::get_cost_transitional(const OperatorProxy &op) const {
    /*
      This method is only intended for the transitional period while we still
      support the "old style" of adjusting costs for the heuristics (via the
      cost_type parameter) in parallel with the "new style" (via task
      transformations). Once all heuristics are adapted to support task
      transformations and we can remove the "cost_type" attribute,
      Heuristic::get_cost_transitional(op) can be replaced by op.get_cost().
    */
    if (op.is_axiom())
        /*
          HACK: We need this to ensure the "old-style" behaviour of
          always using cost 0 for all axioms, but it also means that a
          task transformation cannot use non-zero costs for axioms.
          Once we get rid of cost_type, we can start supporting positive-cost
          axioms if we want (whatever that means).
        */
        return 0;
    else
        return get_adjusted_action_cost(op.get_cost(), cost_type);
}

State Heuristic::convert_global_state(const GlobalState &global_state) const {
    return task->convert_global_state(global_state);
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    parser.add_option<AbstractTask *>(
        "task",
        "Task that the heuristic should operate on. Choose from global_task and adapt_costs.",
        "global_task");
}

//this solution to get default values seems not optimal:
Options Heuristic::default_options() {
    Options opts = Options();
    opts.set<AbstractTask *>("task", new RootTask());
    opts.set<int>("cost_type", 0);
    return opts;
}
