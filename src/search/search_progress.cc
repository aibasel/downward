#include "search_progress.h"

#include "evaluation_context.h"
#include "evaluator.h"

#include "utils/logging.h"

#include <iostream>
#include <string>

using namespace std;

bool SearchProgress::process_evaluator_value(const Evaluator *evaluator, int value) {
    /*
      Handle one evaluator value:
      1. insert into or update min_values if necessary
      2. return true if this is a new lowest value
         (includes case where we haven't seen this evaluator before)
    */
    auto insert_result = min_values.insert(make_pair(evaluator, value));
    auto iter = insert_result.first;
    bool was_inserted = insert_result.second;
    if (was_inserted) {
        // We haven't seen this evaluator before.
        return true;
    } else {
        int &min_value = iter->second;
        if (value < min_value) {
            min_value = value;
            return true;
        }
    }
    return false;
}

bool SearchProgress::check_progress(const EvaluationContext &eval_context) {
    bool boost = false;
    eval_context.get_cache().for_each_evaluator_result(
        [this, &boost](const Evaluator *eval, const EvaluationResult &result) {
            if (eval->is_used_for_reporting_minima() || eval->is_used_for_boosting()) {
                if (process_evaluator_value(eval, result.get_evaluator_value())) {
                    if (eval->is_used_for_reporting_minima()) {
                        eval->report_new_minimum_value(result);
                    }
                    if (eval->is_used_for_boosting()) {
                        boost = true;
                    }
                }
            }
        }
        );
    return boost;
}
