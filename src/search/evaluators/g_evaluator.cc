#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

namespace g_evaluator {
EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_h_value(eval_context.get_g_value());
    return result;
}

static Evaluator *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "g-value evaluator",
        "Returns the g-value (path cost) of the search node.");
    parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new GEvaluator;
}

static Plugin<Evaluator> _plugin("g", _parse);
}
