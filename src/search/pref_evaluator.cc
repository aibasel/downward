#include "pref_evaluator.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "option_parser.h"
#include "plugin.h"


PrefEvaluator::PrefEvaluator() {
}

PrefEvaluator::~PrefEvaluator() {
}

EvaluationResult PrefEvaluator::compute_result(
    EvaluationContext &eval_context) {
    EvaluationResult result;
    if (eval_context.is_preferred())
        result.set_h_value(0);
    else
        result.set_h_value(1);
    return result;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis("Preference evaluator",
                             "Returns 0 if preferred is true and 1 otherwise.");
    parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new PrefEvaluator;
}

static Plugin<ScalarEvaluator> _plugin("pref", _parse);
