#include "pref_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace pref_evaluator {
PrefEvaluator::PrefEvaluator() {
}

PrefEvaluator::~PrefEvaluator() {
}

EvaluationResult PrefEvaluator::compute_result(
    EvaluationContext &eval_context) {
    EvaluationResult result;
    if (eval_context.is_preferred())
        result.set_evaluator_value(0);
    else
        result.set_evaluator_value(1);
    return result;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis("Preference evaluator",
                             "Returns 0 if preferred is true and 1 otherwise.");
    parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<PrefEvaluator>();
}

static Plugin<Evaluator> _plugin("pref", _parse, "evaluators_basic");
}
