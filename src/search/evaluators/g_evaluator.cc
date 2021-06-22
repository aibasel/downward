#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../tasks/root_task.h"

using namespace std;

namespace g_evaluator {
static const int INFTY = numeric_limits<int>::max();

GEvaluator::GEvaluator(const shared_ptr<AbstractTask> &task)
    : task(task),
      task_proxy(*task),
      g_values(INFTY) {
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(g_values[eval_context.get_state()]);
    return result;
}

void GEvaluator::get_path_dependent_evaluators(std::set<Evaluator *> &evals) {
    evals.insert(this);
}

void GEvaluator::notify_initial_state(const State &initial_state) {
    g_values[initial_state] = 0;
}

void GEvaluator::notify_state_transition(
    const State &parent_state, OperatorID op_id, const State &state) {
    g_values[state] = min(
        g_values[state],
        g_values[parent_state] + task_proxy.get_operators()[op_id.get_index()].get_cost());
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "g-value evaluator",
        "Returns the g-value (path cost) of the search node.");
    parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GEvaluator>(tasks::g_root_task);
}

static Plugin<Evaluator> _plugin("g", _parse, "evaluators_basic");
}
