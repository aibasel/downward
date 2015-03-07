#include "evaluation_context.h"

#include "heuristic.h"

#include <cassert>

using namespace std;


const EvaluationContext::HeuristicResult &
EvaluationContext::get_result(Heuristic *heur) {
    HeuristicResult &result = heuristic_results[heur];
    if (result.is_uninitialized()) {
        heur->evaluate(state);
        if (heur->is_dead_end())
            result.h_value = INFINITE;
        else
            result.h_value = heur->get_value();
        heur->get_preferred_operators(result.preferred_operators);
    }
    return result;
}

EvaluationContext::EvaluationContext(const GlobalState &state)
    : state(state) {
}

void EvaluationContext::evaluate_heuristic(Heuristic *heur) {
    get_result(heur);
}

bool EvaluationContext::is_heuristic_infinite(Heuristic *heur) {
    return get_result(heur).is_heuristic_infinite();
}

int EvaluationContext::get_heuristic_value(Heuristic *heur) {
    int h = get_result(heur).h_value;
    assert(h != INFINITE);
    return h;
}

int EvaluationContext::get_heuristic_value_or_infinity(Heuristic *heur) {
    return get_result(heur).h_value;
}

const vector<const GlobalOperator *> &
EvaluationContext::get_preferred_operators(Heuristic *heur) {
    return get_result(heur).preferred_operators;
}

bool EvaluationContext::is_dead_end() const {
    bool all_estimates_are_infinite = true;
    for (const auto &entry : heuristic_results) {
        const Heuristic *heur = entry.first;
        const HeuristicResult &result = entry.second;
        if (result.is_heuristic_infinite()) {
            if (heur->dead_ends_are_reliable())
                return true;
        } else {
            all_estimates_are_infinite = false;
        }
    }
    return all_estimates_are_infinite && !heuristic_results.empty();
}
