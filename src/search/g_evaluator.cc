#include "g_evaluator.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "option_parser.h"
#include "plugin.h"

GEvaluator::GEvaluator() {
}

GEvaluator::~GEvaluator() {
}

bool GEvaluator::dead_end_is_reliable() const {
    return true;
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_h_value(eval_context.get_g_value());
    return result;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis("g-value evaluator",
                             "Returns the current g-value of the search.");
    parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new GEvaluator;
}

static Plugin<ScalarEvaluator> _plugin("g", _parse);
