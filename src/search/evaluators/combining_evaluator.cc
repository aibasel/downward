#include "combining_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../option_parser.h"

using namespace std;

namespace combining_evaluator {
CombiningEvaluator::CombiningEvaluator(
    const options::Options &opts)
    : Evaluator(opts),
      subevaluators(opts.get_list<shared_ptr<Evaluator>>("evals")) {
    all_dead_ends_are_reliable = true;
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators)
        if (!subevaluator->dead_ends_are_reliable())
            all_dead_ends_are_reliable = false;
}

CombiningEvaluator::~CombiningEvaluator() {
}

bool CombiningEvaluator::dead_ends_are_reliable() const {
    return all_dead_ends_are_reliable;
}

EvaluationResult CombiningEvaluator::compute_result(
    EvaluationContext &eval_context) {
    // This marks no preferred operators.
    EvaluationResult result;
    vector<int> values;
    values.reserve(subevaluators.size());

    // Collect component values. Return infinity if any is infinite.
    for (const shared_ptr<Evaluator> &subevaluator : subevaluators) {
        int value = eval_context.get_evaluator_value_or_infinity(subevaluator.get());
        if (value == EvaluationResult::INFTY) {
            result.set_evaluator_value(value);
            return result;
        } else {
            values.push_back(value);
        }
    }

    // If we arrived here, all subevaluator values are finite.
    result.set_evaluator_value(combine_values(values));
    return result;
}

void CombiningEvaluator::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (auto &subevaluator : subevaluators)
        subevaluator->get_path_dependent_evaluators(evals);
}
void add_combining_evaluator_options_to_parser(options::OptionParser &parser) {
    parser.add_list_option<shared_ptr<Evaluator>>(
        "evals", "at least one evaluator");
    add_evaluator_options_to_parser(parser);
}
}
