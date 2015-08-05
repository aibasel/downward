#include "heuristic.h"

#include "cost_adapted_task.h"
#include "evaluation_context.h"
#include "evaluation_result.h"
#include "global_operator.h"
#include "globals.h"
#include "option_parser.h"
#include "operator_cost.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

Heuristic::Heuristic(const Options &opts)
    : description(opts.get_unparsed_config()),
      initialized(false),
      task(get_task_from_options(opts)),
      task_proxy(*task),
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

bool Heuristic::reach_state(
    const GlobalState & /*parent_state*/,
    const GlobalOperator & /*op*/,
    const GlobalState & /*state*/) {
    return false;
}

int Heuristic::get_adjusted_cost(const GlobalOperator &op) const {
    return get_adjusted_action_cost(op, cost_type);
}

State Heuristic::convert_global_state(const GlobalState &global_state) const {
    return task_proxy.convert_global_state(global_state);
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    // TODO: When the cost_type option is gone, use "no_transform" as default.
    parser.add_option<shared_ptr<AbstractTask> >(
        "transform",
        "Optional task transformation for the heuristic. "
        "Currently only adapt_costs is available.",
        OptionParser::NONE);
}

//this solution to get default values seems not optimal:
Options Heuristic::default_options() {
    Options opts = Options();
    opts.set<shared_ptr<AbstractTask> >("transform", g_root_task());
    opts.set<int>("cost_type", NORMAL);
    return opts;
}

EvaluationResult Heuristic::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    if (!initialized) {
        initialize();
        initialized = true;
    }

    assert(preferred_operators.empty());

    const GlobalState &state = eval_context.get_state();
    int heuristic = compute_heuristic(state);
    for (const GlobalOperator *preferred_operator : preferred_operators)
        preferred_operator->unmark();
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
    result.set_preferred_operators(move(preferred_operators));
    assert(preferred_operators.empty());
    return result;
}

string Heuristic::get_description() const {
    return description;
}
