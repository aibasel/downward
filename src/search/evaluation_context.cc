#include "evaluation_context.h"

#include "evaluation_result.h"
#include "heuristic.h"
#include "scalar_evaluator.h"

#include <cassert>

using namespace std;


EvaluationContext::EvaluationContext(
    const GlobalState &state, int g, bool is_preferred)
    : state(state),
      g_value(g),
      preferred(is_preferred) {
}

const EvaluationResult &EvaluationContext::get_result(ScalarEvaluator *heur) {
    EvaluationResult &result = eval_results[heur];
    if (result.is_uninitialized())
        result = heur->compute_result(*this);
    return result;
}

const GlobalState &EvaluationContext::get_state() const {
    return state;
}

void EvaluationContext::evaluate_heuristic(ScalarEvaluator *heur) {
    get_result(heur);
}

void EvaluationContext::hacky_set_evaluator_value(ScalarEvaluator *heur, int value) {
    eval_results[heur].set_h_value(value);
}

int EvaluationContext::get_g_value() const {
    return g_value;
}

bool EvaluationContext::is_preferred() const {
    return preferred;
}

bool EvaluationContext::is_heuristic_infinite(ScalarEvaluator *heur) {
    return get_result(heur).is_infinite();
}

int EvaluationContext::get_heuristic_value(ScalarEvaluator *heur) {
    int h = get_result(heur).get_h_value();
    assert(h != EvaluationResult::INFINITE);
    return h;
}

int EvaluationContext::get_heuristic_value_or_infinity(ScalarEvaluator *heur) {
    return get_result(heur).get_h_value();
}

const vector<const GlobalOperator *> &
EvaluationContext::get_preferred_operators(ScalarEvaluator *heur) {
    return get_result(heur).get_preferred_operators();
}

bool EvaluationContext::is_dead_end() const {
    bool all_estimates_are_infinite = true;
    bool at_least_one_estimate = false;
    for (const auto &entry : eval_results) {
        const ScalarEvaluator *heur = entry.first;
        const EvaluationResult &result = entry.second;
        const Heuristic *h = dynamic_cast<const Heuristic *>(heur);
        if (h != nullptr) {
            /* Only consider actual heuristics.
               TODO: Get rid of the cast and test once we've unified
               Heuristic and ScalarEvaluator. */
            if (result.is_infinite()) {
                if (h->dead_ends_are_reliable())
                    return true;
            } else {
                all_estimates_are_infinite = false;
            }
        }
    }
    return at_least_one_estimate && all_estimates_are_infinite;
}
