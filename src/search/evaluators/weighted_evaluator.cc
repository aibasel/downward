#include "weighted_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace weighted_evaluator {
WeightedEvaluator::WeightedEvaluator(const Options &opts)
    : evaluator(opts.get<Evaluator *>("eval")),
      w(opts.get<int>("weight")) {
}

WeightedEvaluator::WeightedEvaluator(Evaluator *eval, int weight)
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
    if (h_val != EvaluationResult::INFTY) {
        // TODO: Check for overflow?
        h_val *= w;
    }
    result.set_h_value(h_val);
    return result;
}

void WeightedEvaluator::get_involved_heuristics(set<Heuristic *> &hset) {
    evaluator->get_involved_heuristics(hset);
}

static Evaluator *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Weighted evaluator",
        "Multiplies the value of the evaluator with the given weight.");
    parser.add_option<Evaluator *>("eval", "evaluator");
    parser.add_option<int>("weight", "weight");
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return new WeightedEvaluator(opts);
}

static Plugin<Evaluator> _plugin("weight", _parse);
}
