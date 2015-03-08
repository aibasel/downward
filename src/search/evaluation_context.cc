#include "evaluation_context.h"

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

const EvaluationContext::HeuristicResult &
EvaluationContext::get_result(ScalarEvaluator *heur) {
    HeuristicResult &result = heuristic_results[heur];
    if (result.is_uninitialized()) {
        /*
          TODO: This code is currently messy because we have to
          support both heuristics and other kinds of scalar
          evaluators. This messiness is temporary. See the general
          comments in the header file.
        */

        Heuristic *h = dynamic_cast<Heuristic *>(heur);
        if (h != nullptr) {
            // We have an actual heuristic.
            h->evaluate(state);
        } else {
            // We have non-heuristic scalar evaluator.
            heur->evaluate(g_value, preferred);
        }
        if (heur->is_dead_end())
            result.h_value = INFINITE;
        else
            result.h_value = heur->get_value();
        if (h != nullptr) {
            // We have an actual heuristic.
            h->get_preferred_operators(result.preferred_operators);
        }
    }
    return result;
}

void EvaluationContext::evaluate_heuristic(ScalarEvaluator *heur) {
    get_result(heur);
}

void EvaluationContext::hacky_set_evaluator_value(ScalarEvaluator *heur, int value) {
    heuristic_results[heur].h_value = value;
}

int EvaluationContext::get_g_value() const {
    return g_value;
}

bool EvaluationContext::is_preferred() const {
    return preferred;
}

bool EvaluationContext::is_heuristic_infinite(ScalarEvaluator *heur) {
    return get_result(heur).is_heuristic_infinite();
}

int EvaluationContext::get_heuristic_value(ScalarEvaluator *heur) {
    int h = get_result(heur).h_value;
    assert(h != INFINITE);
    return h;
}

int EvaluationContext::get_heuristic_value_or_infinity(ScalarEvaluator *heur) {
    return get_result(heur).h_value;
}

const vector<const GlobalOperator *> &
EvaluationContext::get_preferred_operators(ScalarEvaluator *heur) {
    return get_result(heur).preferred_operators;
}

bool EvaluationContext::is_dead_end() const {
    bool all_estimates_are_infinite = true;
    bool at_least_one_estimate = false;
    for (const auto &entry : heuristic_results) {
        const ScalarEvaluator *heur = entry.first;
        const HeuristicResult &result = entry.second;
        const Heuristic *h = dynamic_cast<const Heuristic *>(heur);
        if (h != nullptr) {
            /* Only consider actual heuristics.
               TODO: Get rid of the cast and test once we've unified
               Heuristic and ScalarEvaluator. */
            if (result.is_heuristic_infinite()) {
                if (h->dead_ends_are_reliable())
                    return true;
            } else {
                all_estimates_are_infinite = false;
            }
        }
    }
    return at_least_one_estimate && all_estimates_are_infinite;
}
