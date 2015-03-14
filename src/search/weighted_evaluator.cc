#include "weighted_evaluator.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "option_parser.h"
#include "plugin.h"

#include <cstdlib>
#include <sstream>

WeightedEvaluator::WeightedEvaluator(const Options &opts)
    : evaluator(opts.get<ScalarEvaluator *>("eval")),
      w(opts.get<int>("weight")) {
}

WeightedEvaluator::WeightedEvaluator(ScalarEvaluator *eval, int weight)
    : evaluator(eval), w(weight) {
}


WeightedEvaluator::~WeightedEvaluator() {
}

bool WeightedEvaluator::dead_ends_are_reliable() const {
    return evaluator->dead_ends_are_reliable();
}

EvaluationResult WeightedEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // Note that this produces no preferred operators.
    EvaluationResult result;
    int h_val = eval_context.get_heuristic_value_or_infinity(evaluator);
    if (h_val != EvaluationResult::INFINITE) {
        // TODO: Check for overflow?
        h_val *= w;
    }
    result.set_h_value(h_val);
    return result;
}

void WeightedEvaluator::get_involved_heuristics(std::set<Heuristic *> &hset) {
    evaluator->get_involved_heuristics(hset);
}

static ScalarEvaluator *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Weighted evaluator",
        "Multiplies the value of the scalar evaluator with the given weight.");
    parser.add_option<ScalarEvaluator *>("eval", "scalar evaluator");
    parser.add_option<int>("weight", "weight");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new WeightedEvaluator(opts);
}

static Plugin<ScalarEvaluator> _plugin("weight", _parse);
