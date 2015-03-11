#include "heuristic.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "global_operator.h"
#include "option_parser.h"
#include "operator_cost.h"
#include "task_proxy.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

Heuristic::Heuristic(const Options &opts)
    : initialized(false),
      task(opts.get<TaskProxy *>("task")),
      cost_type(OperatorCost(opts.get_enum("cost_type"))) {
}

Heuristic::~Heuristic() {
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

void Heuristic::get_preferred_operators(std::vector<const GlobalOperator *> &result) {
    result.insert(result.end(),
                  preferred_operators.begin(),
                  preferred_operators.end());
}

bool Heuristic::reach_state(const GlobalState & /*parent_state*/,
                            const GlobalOperator & /*op*/, const GlobalState & /*state*/) {
    return false;
}

bool Heuristic::dead_end_is_reliable() const {
    return dead_ends_are_reliable();
}

int Heuristic::get_adjusted_cost(const GlobalOperator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

int Heuristic::get_adjusted_cost(const OperatorProxy &op) const {
    if (op.is_axiom())
        return 0;
    else
        return get_adjusted_action_cost(op.get_cost(), cost_type);
}

State Heuristic::convert_global_state(const GlobalState &global_state) const {
    return task->convert_global_state(global_state);
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    parser.add_option<TaskProxy *>(
        "task",
        "Task that the heuristic should operate on. Currently only global_task is supported.",
        "global_task");
}

//this solution to get default values seems not optimal:
Options Heuristic::default_options() {
    Options opts = Options();
    opts.set<TaskProxy *>("task", 0);  // TODO: Use correct task.
    opts.set<int>("cost_type", 0);
    return opts;
}

EvaluationResult Heuristic::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    if (!initialized) {
        initialize();
        initialized = true;
    }

    preferred_operators.clear();
    const GlobalState &state = eval_context.get_state();
    int heuristic = compute_heuristic(state);
    for (size_t i = 0; i < preferred_operators.size(); ++i)
        preferred_operators[i]->unmark();
    assert(heuristic == DEAD_END || heuristic >= 0);

    if (heuristic == DEAD_END) {
        /*
          It is permissible to mark preferred operators for dead-end
          states (thus allowing a heuristic to mark them on-the-fly
          before knowing the final result), but if it turns out we
          have a dead end, we don't want to actually report any
          preferred operators.
        */
        preferred_operators.clear();
        heuristic = EvaluationResult::INFINITE;
    }

#ifndef NDEBUG
    if (heuristic != EvaluationResult::INFINITE) {
        for (size_t i = 0; i < preferred_operators.size(); ++i)
            assert(preferred_operators[i]->is_applicable(state));
    }
#endif

    result.set_h_value(heuristic);
    vector<const GlobalOperator *> pref_ops;
    get_preferred_operators(pref_ops);
    result.set_preferred_operators(pref_ops);
    return result;
}
