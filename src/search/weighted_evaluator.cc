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

bool WeightedEvaluator::dead_end_is_reliable() const {
    return evaluator->dead_end_is_reliable();
}

EvaluationResult WeightedEvaluator::compute_result(
    EvaluationContext &eval_context) {
    EvaluationResult result = evaluator->compute_result(eval_context);
    /*
      TODO/NOTE: Note that this copies the preferred operators of the
      subevaluator, too. (Our previous implementation of this was a
      ScalarEvaluator, which could not have preferred operators.)

      It is not yet fully clear how we ultimately want to handle
      preferred operators in the new
      EvaluationResult/EvaluationContext framework; in particular, if
      we want to keep them as part of each EvaluationResult or only
      have a single set of preferred operators in the whole
      EvaluationContext object. But I think the latter probably won't
      work easily if we want to do something like "use alternation
      between FF and CG heuristic, use only FF's preferred operators,
      and only compute each heuristic once.
    */
    if (!result.is_infinite()) {
        // TODO: catch overflow?
        result.set_h_value(w * result.get_h_value());
    }
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
