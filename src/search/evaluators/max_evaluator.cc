#include "max_evaluator.h"

#include "../evaluation_context.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>

using namespace std;

namespace max_evaluator {
MaxEvaluator::MaxEvaluator(const Options &opts)
    : CombiningEvaluator(opts.get_list<ScalarEvaluator *>("evals")) {
}

MaxEvaluator::~MaxEvaluator() {
}

int MaxEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result = max(result, value);
    }
    return result;
}

EvaluationResult MaxEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // This marks no preferred operators.
    EvaluationResult result;

    int value = 0;
    for (ScalarEvaluator *subevaluator : subevaluators) {
        int h_val = eval_context.get_heuristic_value_or_infinity(subevaluator);
        if (h_val == EvaluationResult::INFTY) {
            value = h_val;
            break;
        } else {
            value = max(value, h_val);
        }
    }
    result.set_h_value(value);
    return result;
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Max evaluator",
        "Calculates the maximum of the sub-evaluators.");
    parser.add_list_option<ScalarEvaluator *>(
        "evals",
        "at least one scalar evaluator");

    Options opts = parser.parse();

    opts.verify_list_non_empty<ScalarEvaluator *>("evals");

    if (parser.dry_run()) {
        return nullptr;
    }
    return new MaxEvaluator(opts);
}

static Plugin<ScalarEvaluator> plugin("max", _parse);
}
