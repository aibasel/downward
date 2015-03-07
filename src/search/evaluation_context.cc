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

bool EvaluationContext::is_dead_end(Heuristic *heur) {
    return get_result(heur).is_dead_end();
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
