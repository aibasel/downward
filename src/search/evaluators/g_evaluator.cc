#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluator::GEvaluator(const options::Options &opts)
    : Evaluator(opts) {
}

EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}

static shared_ptr<Evaluator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "g-value evaluator",
        "Returns the g-value (path cost) of the search node.");
    add_evaluator_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GEvaluator>(opts);
}

static Plugin<Evaluator> _plugin("g", _parse, "evaluators_basic");
}
